#include <traceroute.h>

int main(int argc, const char * const * argv) {
  TR_Options options = TR_parseArgs(argc, argv);

  if (options.global_opts & TR_HELP)
    return TR_help();

  return TR_default(&options);
}
