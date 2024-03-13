#include <traceroute.h>

int TR_run(const TR_Options * options, const TR_Driver * driver) {
  TR_SocketSet tr = {
      .nfds = options->maxTtl - options->firstTtl + 1,
  };

  if (TR_initSockets(&tr, options, driver) ||
      TR_processRequests(&tr, options, driver) ||
      TR_processResponses(&tr, options, driver))
    goto Error;

  TR_log(tr.sockets, options, tr.lastHop + 1);
  TR_cleanupSockets(&tr);
  return TR_SUCCESS;

Error:
  perror("ft_traceroute");
  TR_cleanupSockets(&tr);
  return TR_FAILURE;
}
