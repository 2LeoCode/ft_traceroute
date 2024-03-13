#include <traceroute.h>

void TR_log(
    const TR_Socket * sockets, const TR_Options * options, size_t hops
) {
  char dstIp[INET_ADDRSTRLEN + 1] = {};
  inet_ntop(
      AF_INET,
      (struct in_addr[]){{.s_addr = options->dstAddress}},
      dstIp,
      INET_ADDRSTRLEN
  );
  printf(
      "traceroute to %s (%s), %d hops max, %d byte packets\n",
      options->dstHost,
      dstIp,
      options->maxTtl,
      options->packetLen
  );

  char ip[INET_ADDRSTRLEN + 1];
  char name[NI_MAXHOST + 1];
  for (uint8_t i = 0; i < hops; ++i) {
    printf("%2d", i + options->firstTtl);
    for (uint8_t j = 0; j < options->nQueries; ++j) {
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
            " %s (%s) %.3lf ms",
            name,
            ip,
            TR_chronoElapsedMs(sockets[i].chronos + j)
        );
      }
    }
    printf("\n");
  }
}
