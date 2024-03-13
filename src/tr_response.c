#include <traceroute.h>

static void TR_processTimeouts(
    TR_SocketSet * tr, const TR_Options * options, uint8_t hopIdx
) {
  for (uint8_t j = 0; j < options->nQueries; ++j) {
    if (!tr->sockets[hopIdx].chronos[j].status) {
      double timeoutMs = options->wait.max * 1000;
      bool foundHere = false;

      if (options->wait.here) {
        for (uint8_t k = 0; k < options->nQueries; ++k) {
          if (tr->sockets[hopIdx].chronos[k].status == TR_CHRONO_SUCCESS) {
            timeoutMs = TR_chronoElapsedMs(tr->sockets[hopIdx].chronos + k) *
                        options->wait.here;
            foundHere = true;
            break;
          }
        }
      }
      if (!foundHere && options->wait.near) {
        for (uint8_t k = hopIdx + 1; k <= tr->lastHop; ++k) {
          for (uint8_t l = 0; l < options->nQueries; ++l) {
            if (tr->sockets[k].chronos[l].status == TR_CHRONO_SUCCESS) {
              timeoutMs = TR_chronoElapsedMs(tr->sockets[k].chronos + l) *
                          options->wait.near;
              goto found;
            }
          }
        }
      }
    found:
      if (TR_chronoElapsedMs(tr->sockets[hopIdx].chronos + j) > timeoutMs) {
        tr->sockets[hopIdx].chronos[j].status = TR_CHRONO_TIMEOUT;
        ++tr->sockets[hopIdx].packetsReceivedOrLost;
      }
    }
  }
}

int TR_processResponses(TR_SocketSet * tr, const TR_Options * options) {
  while (tr->nfds != 0) {
    fd_set rfd = tr->fds;

    if (select(FD_SETSIZE, &rfd, NULL, NULL, (struct timeval[]){{}}) == -1)
      return TR_FAILURE;

    for (uint8_t i = 0; i <= tr->lastHop; ++i) {
      if (FD_ISSET(tr->sockets[i].fileno, &rfd)) {
        char buf[1024];
        // received a normal udp response (shouldn't happen but it still
        // means that we reached the destination)

        const uint8_t idx = tr->sockets[i].packetsReceivedOrLost;

        // idx should always be less than param->nQueries

        ssize_t recvRet = recvfrom(
            tr->sockets[i].fileno,
            buf,
            sizeof(buf),
            0,
            (void *)(tr->sockets[i].responseAddresses + idx),
            (socklen_t[]){sizeof(struct sockaddr_in)}
        );

        TR_chronoStop(tr->sockets[i].chronos + idx);
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

          if (recvmsg(tr->sockets[i].fileno, &msg, MSG_ERRQUEUE) == -1)
            return TR_FAILURE;
          for (cmsg = CMSG_FIRSTHDR(&msg); cmsg;
               cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVERR) {
              // received an error message
              struct sock_extended_err * sockError = (void *)CMSG_DATA(cmsg);
              if (sockError && sockError->ee_origin == SO_EE_ORIGIN_ICMP) {
                // received an icmp error message
                tr->sockets[i].responseAddresses[idx] =
                    *(struct sockaddr_in *)SO_EE_OFFENDER(sockError);
                switch (sockError->ee_type) {
                  case ICMP_TIME_EXCEEDED:
                    // intermediary hop
                    break;
                  case ICMP_DEST_UNREACH:
                    // destination reached

                    for (uint8_t j = i + 1; j <= tr->lastHop; ++j) {
                      FD_CLR(tr->sockets[j].fileno, &tr->fds);
                      close(tr->sockets[j].fileno);
                      tr->sockets[j].fileno = 0;
                      --tr->nfds;
                    }
                    tr->lastHop = i;
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
          // that we've reached the last hop)

          for (uint8_t j = i + 1; j <= tr->lastHop; ++j) {
            FD_CLR(tr->sockets[j].fileno, &tr->fds);
            close(tr->sockets[j].fileno);
            tr->sockets[j].fileno = 0;
            --tr->nfds;
          }
          tr->lastHop = i;
        }
        ++tr->sockets[i].packetsReceivedOrLost;
      }
    }
    for (uint8_t i = 0; i <= tr->lastHop; ++i) {
      TR_processTimeouts(tr, options, i);
      if (tr->sockets[i].packetsReceivedOrLost == options->nQueries &&
          FD_ISSET(tr->sockets[i].fileno, &tr->fds)) {
        FD_CLR(tr->sockets[i].fileno, &tr->fds);
        close(tr->sockets[i].fileno);
        tr->sockets[i].fileno = 0;
        --tr->nfds;
      }
    }
  }
  return TR_SUCCESS;
}
