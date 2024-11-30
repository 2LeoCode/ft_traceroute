#include <traceroute.h>

int TR_processRequests(TR_SocketSet *tr, const TR_Options *options,
                       const TR_Driver *driver) {
  TR_Packet *pkt = driver->buildPacket(options->packetLen);
  if (!pkt)
    return TR_FAILURE;

  for (uint8_t i = 0; i <= tr->lastHop; ++i) {
    for (uint8_t j = 0; j < options->nQueries; ++j) {
      tr->sockets[i].dstAddress.sin_port = htons(options->port + j);
      if (driver->send(tr->sockets + i, pkt) == -1)
        goto Error;
      TR_chronoStart(tr->sockets[i].chronos + j);
    }
  }
  free(pkt);
  return TR_SUCCESS;

Error:
  free(pkt);
  return TR_FAILURE;
}
