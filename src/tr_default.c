#include <traceroute.h>

int TR_default(const TR_Options * options) {
  TR_SocketSet tr = {
      .nfds = options->maxTtl - options->firstTtl + 1,
      .domain = AF_INET,
      .type = SOCK_DGRAM | SOCK_NONBLOCK,
      .protocol = IPPROTO_UDP,
  };

  if (TR_initSockets(&tr, options) || TR_processRequests(&tr, options) ||
      TR_processResponses(&tr, options))
    goto Error;

  TR_log(tr.sockets, options, tr.lastHop + 1);
  TR_cleanupSockets(&tr);
  return TR_SUCCESS;

Error:
  perror("ft_traceroute");
  TR_cleanupSockets(&tr);
  return TR_FAILURE;
}
