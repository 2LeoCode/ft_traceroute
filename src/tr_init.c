#include <traceroute.h>

int TR_initSockets(
    TR_SocketSet * tr, const TR_Options * options, const TR_Driver * driver
) {
  tr->lastHop = options->maxTtl - options->firstTtl;
  tr->nfds = tr->lastHop + 1;
  FD_ZERO(&tr->fds);

  for (uint8_t i = 0; i < tr->nfds; ++i) {
    tr->sockets[i] = (TR_Socket){
        .fileno = socket(
            driver->domain, driver->type | SOCK_NONBLOCK, driver->protocol
        ),
    };
    if (tr->sockets[i].fileno == -1)
      return TR_FAILURE;

    if (setsockopt(
            tr->sockets[i].fileno,
            SOL_IP,
            IP_TTL,
            (int[]){i + options->firstTtl},
            sizeof(int)
        ) ||
        setsockopt(
            tr->sockets[i].fileno, SOL_IP, IP_RECVERR, (int[]){1}, sizeof(int)
        ))
      return TR_FAILURE;

    FD_SET(tr->sockets[i].fileno, &tr->fds);

    tr->sockets[i].dstAddress = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_addr =
            {
                .s_addr = options->dstAddress,
            },
    };
  }
  return TR_SUCCESS;
}
