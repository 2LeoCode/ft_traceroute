#include <traceroute.h>

static void TR_processTimeouts(TR_SocketSet *tr, const TR_Options *options,
                               uint8_t hopIdx) {
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

int TR_processResponses(TR_SocketSet *tr, const TR_Options *options,
                        const TR_Driver *driver) {
  while (tr->nfds != 0) {
    fd_set rfd = tr->fds;

    if (select(FD_SETSIZE, &rfd, NULL, NULL, (struct timeval[]){{}}) == -1)
      return TR_FAILURE;

    for (uint8_t i = 0; i <= tr->lastHop; ++i) {
      if (FD_ISSET(tr->sockets[i].fileno, &rfd)) {
        bool dstReached;
        if (driver->recv(tr->sockets + i, &dstReached))
          return TR_FAILURE;
        if (dstReached) {
          for (uint8_t j = i + 1; j <= tr->lastHop; ++j) {
            FD_CLR(tr->sockets[j].fileno, &tr->fds);
            close(tr->sockets[j].fileno);
            tr->sockets[j].fileno = 0;
            --tr->nfds;
          }
          tr->lastHop = i;
        }
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
