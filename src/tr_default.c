#include <arpa/inet.h>
#include <linux/errqueue.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <traceroute.h>
#include <unistd.h>

static void TR_socketsCleanup(void * socketsAddress) {
  TR_Socket * sockets = socketsAddress;
  for (size_t i = 0; i < TR_MAX_TTL_MAX; ++i) {
    if (sockets[i].fileno > 0) {
      close(sockets[i].fileno);
      sockets[i].fileno = 0;
    }
  }
}

static void TR_fillPacket(void * pkt, size_t n) {
  for (size_t i = 0; i < n; ++i)
    ((uint8_t *)pkt)[i] = 'a' + (i % 26);
}

int TR_default(const TR_Options * param) {
  jmp_buf env;
  TR_Socket sockets[TR_MAX_TTL_MAX]
      __attribute__((cleanup(TR_socketsCleanup))) = {0};

  uint8_t lastHop = param->maxTtl - param->firstTtl;
  uint8_t nfds = lastHop + 1;
  fd_set fds;
  char * pkt;

  if (setjmp(env) == TR_FAILURE) {
    perror("ft_traceroute");
    free(pkt);
    return TR_FAILURE;
  }
  if (!(pkt = malloc(param->packetLen)))
    longjmp(env, TR_FAILURE);

  TR_fillPacket(pkt, param->packetLen);
  FD_ZERO(&fds);

  for (uint8_t i = 0; i < param->maxTtl; ++i) {
    sockets[i] = (TR_Socket){
        .fileno = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP),
    };
    if (sockets[i].fileno == -1)
      longjmp(env, TR_FAILURE);

    if (setsockopt(
            sockets[i].fileno,
            SOL_IP,
            IP_TTL,
            (int[]){i + param->firstTtl},
            sizeof(int)
        ) ||
        setsockopt(
            sockets[i].fileno, SOL_IP, IP_RECVERR, (int[]){1}, sizeof(int)
        ))
      longjmp(env, TR_FAILURE);

    FD_SET(sockets[i].fileno, &fds);

    sockets[i].dstAddress = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_addr =
            {
                .s_addr = param->dstAddress,
            },
    };
  }

  for (uint8_t i = 0; i < param->nQueries; ++i) {
    const uint16_t port = htons(TR_UDP_UNLIKELY_PORT + i);

    for (uint8_t j = 0; j <= lastHop; ++j) {
      sockets[j].dstAddress.sin_port = port;
      if (sendto(
              sockets[j].fileno,
              pkt,
              param->packetLen,
              0,
              (void *)&sockets[j].dstAddress,
              sizeof(struct sockaddr_in)
          ) == -1)
        longjmp(env, TR_FAILURE);
      TR_chronoStart(sockets[j].chronos + i);
    }
  }

  while (nfds != 0) {
    fd_set rfd = fds;

    if (select(FD_SETSIZE, &rfd, NULL, NULL, (struct timeval[]){{}}) == -1)
      longjmp(env, TR_FAILURE);

    for (uint8_t i = 0; i <= lastHop; ++i) {
      if (FD_ISSET(sockets[i].fileno, &rfd)) {
        char buf[1024];
        // received a normal udp response (shouldn't happen but it still
        // means that we reached the destination)

        const uint8_t idx = sockets[i].packetsReceivedOrLost;

        // idx should always be less than param->nQueries

        ssize_t recvRet = recvfrom(
            sockets[i].fileno,
            buf,
            sizeof(buf),
            0,
            (void *)(sockets[i].responseAddresses + idx),
            (socklen_t[]){sizeof(struct sockaddr_in)}
        );

        TR_chronoStop(sockets[i].chronos + idx);
        if (recvRet == -1) {
          struct msghdr msg = {
              .msg_name = NULL,
              .msg_namelen = 0,
              .msg_iov = NULL,
              .msg_iovlen = 0,
              .msg_flags = 0,
              .msg_control = &buf,
              .msg_controllen = sizeof(buf),
          };
          struct cmsghdr * cmsg;

          if (recvmsg(sockets[i].fileno, &msg, MSG_ERRQUEUE) == -1)
            longjmp(env, TR_FAILURE);
          for (cmsg = CMSG_FIRSTHDR(&msg); cmsg;
               cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVERR) {
              // received an error message
              struct sock_extended_err * sockError = (void *)CMSG_DATA(cmsg);
              if (sockError && sockError->ee_origin == SO_EE_ORIGIN_ICMP) {
                // received an icmp error message
                sockets[i].responseAddresses[idx] =
                    *(struct sockaddr_in *)SO_EE_OFFENDER(sockError);
                switch (sockError->ee_type) {
                  case ICMP_TIME_EXCEEDED:
                    // intermediary hop
                    break;
                  case ICMP_DEST_UNREACH:
                    // destination reached

                    for (uint8_t j = i + 1; j <= lastHop; ++j) {
                      FD_CLR(sockets[j].fileno, &fds);
                      close(sockets[j].fileno);
                      sockets[j].fileno = 0;
                      --nfds;
                    }
                    lastHop = i;
                    break;
                  default:
                    fprintf(
                        stderr,
                        "Invalid icmp error type %d\n",
                        sockError->ee_type
                    );
                    return TR_FAILURE;
                }
              }
            }
          }
        } else {
          // received UDP response (should never happen but it still means
          // that we reached the last hop
          printf("normal msg: %d\n", i);

          for (uint8_t j = i + 1; j <= lastHop; ++j) {
            FD_CLR(sockets[j].fileno, &fds);
            close(sockets[j].fileno);
            sockets[j].fileno = 0;
            --nfds;
          }
          lastHop = i;
        }
        ++sockets[i].packetsReceivedOrLost;
      }
    }

    for (uint8_t i = 0; i <= lastHop; ++i) {
      for (uint8_t j = 0; j < param->nQueries; ++j) {
        if (!sockets[i].chronos[j].status) {
          if (TR_chronoElapsedMs(sockets[i].chronos + j) > TR_TIMEOUT_MS) {
            sockets[i].chronos[j].status = TR_CHRONO_TIMEOUT;
            ++sockets[i].packetsReceivedOrLost;
          }
        }
      }
      if (sockets[i].packetsReceivedOrLost == param->nQueries &&
          FD_ISSET(sockets[i].fileno, &fds)) {
        FD_CLR(sockets[i].fileno, &fds);
        close(sockets[i].fileno);
        sockets[i].fileno = 0;
        --nfds;
      }
    }
  }
  free(pkt);

  char dstIp[INET_ADDRSTRLEN + 1] = {};
  inet_ntop(
      AF_INET,
      (struct in_addr[]){{.s_addr = param->dstAddress}},
      dstIp,
      INET_ADDRSTRLEN
  );
  printf(
      "traceroute to %s (%s), %d hops max, %d byte packets\n",
      param->dstHost,
      dstIp,
      param->maxTtl,
      param->packetLen
  );

  char ip[INET_ADDRSTRLEN + 1];
  char name[NI_MAXHOST + 1];
  for (uint8_t i = 0; i <= lastHop; ++i) {

    printf("%2d", i + param->firstTtl);
    for (uint8_t j = 0; j < 3; ++j) {
      if (sockets[i].chronos[j].status == TR_CHRONO_TIMEOUT)
        printf(" *");
      else if (j && (sockets[i].chronos[j - 1].status == TR_CHRONO_SUCCESS) &&
        sockets[i].responseAddresses[j].sin_addr.s_addr ==
              sockets[i].responseAddresses[j - 1].sin_addr.s_addr)
        printf(" %.3lf ms", TR_chronoElapsedMs(sockets[i].chronos + j));
      else {
        inet_ntop(
            AF_INET,
            &sockets[i].responseAddresses[j].sin_addr,
            ip,
            INET_ADDRSTRLEN
        );
        getnameinfo(
            (struct sockaddr *)&sockets[i].responseAddresses[j],
            sizeof(struct sockaddr_in),
            name,
            NI_MAXHOST,
            NULL,
            0,
            0
        );
        printf(
            " %s (%s) %.5lf ms",
            name,
            ip,
            TR_chronoElapsedMs(sockets[i].chronos + j)
        );
      }
    }
    printf("\n");
  }
  return TR_SUCCESS;
}
