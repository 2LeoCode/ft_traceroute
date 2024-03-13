#include <traceroute.h>

int TR_processRequests(TR_SocketSet * tr, const TR_Options * options) {
  size_t packetLenWithoutIp = options->packetLen - sizeof(struct iphdr);

  uint8_t * pkt = malloc(packetLenWithoutIp);
  if (!pkt)
    return TR_FAILURE;

  for (size_t i = 0; i < packetLenWithoutIp; ++i)
    pkt[i] = 'a' + (i % 26);
  for (uint8_t i = 0; i <= tr->lastHop; ++i) {
    for (uint8_t j = 0; j < options->nQueries; ++j) {
      tr->sockets[i].dstAddress.sin_port = htons(TR_UDP_UNLIKELY_PORT + j);
      if (sendto(
              tr->sockets[i].fileno,
              pkt,
              options->packetLen - sizeof(struct iphdr),
              0,
              (void *)&tr->sockets[i].dstAddress,
              sizeof(struct sockaddr_in)
          ) == -1)
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
