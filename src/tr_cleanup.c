#include <traceroute.h>

void TR_cleanupSockets(TR_SocketSet *tr) {
  for (size_t i = 0; i < TR_MAX_TTL_MAX; ++i) {
    if (tr->sockets[i].fileno > 0) {
      close(tr->sockets[i].fileno);
      tr->sockets[i].fileno = 0;
    }
  }
}
