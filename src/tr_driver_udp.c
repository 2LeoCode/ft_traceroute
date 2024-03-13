#include <traceroute.h>

static TR_Packet * TR_udpBuildPacket(size_t size) {
  const size_t packetSize = size - sizeof(struct iphdr);
  TR_Packet * packet = malloc(sizeof(TR_Packet) + packetSize);
  if (!packet)
    return NULL;

  packet->size = packetSize;

  for (size_t i = 0; i < packet->size; ++i)
    packet->data[i] = 'a' + (i % 26);
  return packet;
}

static int TR_udpSend(const TR_Socket * socket, TR_Packet * packet) {
  return sendto(
      socket->fileno,
      packet->data,
      packet->size,
      0,
      (void *)&socket->dstAddress,
      sizeof(struct sockaddr_in)
  );
}

static int TR_udpRecv(TR_Socket * socket, bool * dstReached) {
  const uint8_t idx = socket->packetsReceivedOrLost;
  char buf[1024];
  ssize_t recvRet = recvfrom(
      socket->fileno,
      buf,
      sizeof(buf),
      0,
      (void *)(socket->responseAddresses + idx),
      (socklen_t[]){sizeof(struct sockaddr_in)}
  );

  *dstReached = false;
  TR_chronoStop(socket->chronos + idx);
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

    if (recvmsg(socket->fileno, &msg, MSG_ERRQUEUE) == -1)
      return TR_FAILURE;
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
      if (cmsg->cmsg_level == SOL_IP && cmsg->cmsg_type == IP_RECVERR) {
        // received an error message
        struct sock_extended_err * sockError = (void *)CMSG_DATA(cmsg);
        if (sockError && sockError->ee_origin == SO_EE_ORIGIN_ICMP) {
          // received an icmp error message
          socket->responseAddresses[idx] =
              *(struct sockaddr_in *)SO_EE_OFFENDER(sockError);
          switch (sockError->ee_type) {
            case ICMP_TIME_EXCEEDED:
              // intermediary hop
              break;
            case ICMP_DEST_UNREACH:
              // destination reached

              *dstReached = true;
              break;
            default:
              fprintf(
                  stderr, "Invalid icmp error type %d\n", sockError->ee_type
              );
              return TR_FAILURE;
          }
        }
      }
    }
  } else {
    // received UDP response (should never happen but it still means
    // that we've reached the last hop)

    *dstReached = true;
  }
  ++socket->packetsReceivedOrLost;
  return TR_SUCCESS;
}

TR_Driver TR_udpDriver(void) {
  return (TR_Driver){
      .domain = AF_INET,
      .type = SOCK_DGRAM,
      .protocol = IPPROTO_UDP,
      .buildPacket = TR_udpBuildPacket,
      .send = TR_udpSend,
      .recv = TR_udpRecv,
  };
}
