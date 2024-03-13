#ifndef TRACEROUTE_H
#define TRACEROUTE_H

#include <arpa/inet.h>
#include <errno.h>
#include <linux/errqueue.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define TR_SUCCESS 0
#define TR_FAILURE 1

#define TR_MAX_TTL_MAX 0xff
#define TR_FIRST_TTL_MAX 0xff
#define TR_WAIT_MAX_DEFAULT 5.
#define TR_WAIT_HERE_DEFAULT 3.
#define TR_WAIT_NEAR_DEFAULT 10.
#define TR_NQUERIES_MAX 0x40
#define TR_PACKET_LEN_MAX 0xffff

#define TR_PACKET_LEN_DEFAULT 0x40
#define TR_FIRST_TTL_DEFAULT 1
#define TR_MAX_TTL_DEFAULT 30
#define TR_NQUERIES_DEFAULT 3

#define TR_UDP_UNLIKELY_PORT 33434

typedef struct s_tr_socket TR_Socket;
typedef struct s_tr_socket_set TR_SocketSet;
typedef struct s_tr_options TR_Options;
typedef struct s_tr_chrono TR_Chrono;
typedef enum e_tr_chrono_status TR_ChronoStatus;
typedef enum e_tr_global_options TR_GlobalOptions;

enum e_tr_chrono_status {
  TR_CHRONO_SUCCESS = 0b01,
  TR_CHRONO_TIMEOUT = 0b10,
};

enum e_tr_global_options {
  TR_HELP = 0b1,
};

struct s_tr_chrono {
  struct timespec time;
  TR_ChronoStatus status;
};

struct s_tr_socket {
  int fileno;
  struct sockaddr_in dstAddress;
  TR_Chrono chronos[TR_MAX_TTL_MAX];
  struct sockaddr_in responseAddresses[TR_NQUERIES_MAX];
  uint8_t packetsReceivedOrLost;
};

struct s_tr_socket_set {
  TR_Socket sockets[TR_MAX_TTL_MAX];
  fd_set fds;
  uint8_t nfds;
  uint8_t lastHop;
  int domain;
  int type;
  int protocol;
};

struct s_tr_options {
  TR_GlobalOptions global_opts;
  uint8_t packetLen;
  uint8_t firstTtl;
  uint8_t maxTtl;
  uint8_t nQueries;
  const char * dstHost;
  in_addr_t dstAddress;
  struct {
    double max, here, near;
  } wait;
};

TR_Options TR_parseArgs(int argc, const char * const * argv);
int TR_help(void);
int TR_default(const TR_Options * param);

int TR_initSockets(TR_SocketSet * tr, const TR_Options * options);
void TR_cleanupSockets(TR_SocketSet * tr);
int TR_processRequests(TR_SocketSet * tr, const TR_Options * options);
int TR_processResponses(TR_SocketSet * tr, const TR_Options * options);
void TR_log(const TR_Socket * sockets, const TR_Options * options, size_t hops);

void TR_chronoStart(TR_Chrono * chrono);
void TR_chronoStop(TR_Chrono * chrono);
double TR_chronoElapsedMs(const TR_Chrono * chrono);

#endif
