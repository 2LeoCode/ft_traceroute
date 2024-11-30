#include <traceroute.h>

int main(int argc, const char *const *argv) {
  TR_Options options = TR_parseArgs(argc, argv);

  if (options.global_opts & TR_HELP)
    return TR_help();

  TR_Driver driver;

  switch (options.method) {
  case TR_UDP:
  default:
    driver = TR_udpDriver();
    break;
  }

  return TR_run(&options, &driver);
}
