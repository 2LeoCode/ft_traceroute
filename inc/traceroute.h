#ifndef TRACEROUTE_H
#define TRACEROUTE_H

#include <netinet/in.h>
#include <time.h>

#define TR_SUCCESS 0
#define TR_FAILURE 1

#define TR_MAX_TTL_MAX 0xff
#define TR_FIRST_TTL_MAX 0xff
#define TR_NQUERIES_MAX 0x40
#define TR_PACKET_LEN_MAX 0xffff

#define TR_PACKET_LEN_DEFAULT 40
#define TR_FIRST_TTL_DEFAULT 1
#define TR_MAX_TTL_DEFAULT 30
#define TR_NQUERIES_DEFAULT 3

#define TR_UDP_UNLIKELY_PORT 33434
#define TR_TIMEOUT_MS 5000

typedef struct s_tr_socket TR_Socket;
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

struct s_tr_options {
  TR_GlobalOptions global_opts;
  uint8_t packetLen;
  uint8_t firstTtl;
  uint8_t maxTtl;
  uint8_t nQueries;
  const char * dstHost;
  in_addr_t dstAddress;
};

TR_Options TR_parseArgs(int argc, const char * const * argv);
int TR_help(void);
int TR_default(const TR_Options * param);

void TR_chronoStart(TR_Chrono * chrono);
void TR_chronoStop(TR_Chrono * chrono);
double TR_chronoElapsedMs(const TR_Chrono * chrono);

#endif
