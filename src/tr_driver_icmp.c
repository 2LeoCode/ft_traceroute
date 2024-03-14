#include <traceroute.h>

static uint16_t TR_pingChecksum(const void * b, int len) {
  const uint16_t * buf = b;
  uint32_t sum = 0;

  for (sum = 0; len > 1; len -= 2)
    sum += *buf++;
  if (len == 1)
    sum += *(uint8_t *)buf;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ~sum;
}

static TR_Packet * TR_icmpBuildPacket(size_t size) {
  const size_t packetSize = size - sizeof(struct iphdr);
  TR_Packet * pkt = malloc(sizeof(TR_Packet) + packetSize);
  if (!pkt)
    return NULL;

  pkt->size = packetSize;

  struct icmphdr * icmp = (void *)pkt->data;

  icmp->type = ICMP_ECHO;
  icmp->code = 0;
  icmp->checksum = 0;
  icmp->un.echo.id = getpid() & 0xffff;
  icmp->un.echo.sequence = 0;

  uint8_t * data = pkt->data + sizeof(struct icmphdr);
  for (size_t i = 0; i < pkt->size - sizeof(struct icmphdr); ++i)
    data[i] = 'a' + (i % 26);
  return pkt;
}

static int TR_icmpSend(
    const TR_Driver * driver,
    TR_Socket * socket,
    TR_Packet * packet,
    uint8_t sequence
) {
  (void)driver;
  struct icmphdr * icmp = (void *)packet->data;

  icmp->un.echo.sequence = sequence;
  icmp->checksum = TR_pingChecksum(packet->data, packet->size);
  return sendto(
      socket->fileno,
      packet->data,
      packet->size,
      0,
      (void *)&socket->dstAddress,
      sizeof(struct sockaddr_in)
  );
}

static int TR_icmpRecv(TR_Socket * socket, bool * dstReached) {
  *dstReached = false;
  char buf[1024];

  do {
    ssize_t recvRet = recvfrom(
        socket->fileno,
        buf,
        sizeof(buf),
        0,
        (void *)(socket->responseAddresses + socket->packetsReceivedOrLost),
        (socklen_t[]){sizeof(struct sockaddr_in)}
    );
    if (recvRet == -1) {
      if (errno == EHOSTUNREACH)
        goto Success;
      if (errno != EAGAIN)
        return TR_FAILURE;
    }
  } while (errno == EAGAIN);

  *dstReached = true;

Success:
  TR_chronoStop(socket->chronos + socket->packetsReceivedOrLost);
  ++socket->packetsReceivedOrLost;
  return TR_SUCCESS;
}

TR_Driver TR_icmpDriver(void) {
  return (TR_Driver){
      .domain = AF_INET,
      .type = SOCK_RAW,
      .protocol = IPPROTO_ICMP,
      .buildPacket = TR_icmpBuildPacket,
      .send = TR_icmpSend,
      .recv = TR_icmpRecv,
  };
}
