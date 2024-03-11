#include <assert.h>
#include <traceroute.h>

void TR_chronoStart(TR_Chrono * chrono) {
  clock_gettime(CLOCK_MONOTONIC, &chrono->time);
  chrono->status = 0;
}

void TR_chronoStop(TR_Chrono * chrono) {
  if (chrono->status == TR_CHRONO_SUCCESS) {
    return;
  }

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  chrono->time = (struct timespec){
      .tv_sec = now.tv_sec - chrono->time.tv_sec,
      .tv_nsec = now.tv_nsec - chrono->time.tv_nsec,
  };
  chrono->status = TR_CHRONO_SUCCESS;
}

double TR_chronoElapsedMs(const TR_Chrono * chrono) {
  if (chrono->status == TR_CHRONO_SUCCESS) {
    return chrono->time.tv_sec * 1000 + chrono->time.tv_nsec / 1000000;
  }

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  return (now.tv_sec - chrono->time.tv_sec) * 1000 +
         (now.tv_nsec - chrono->time.tv_nsec) / 1000000;
}
