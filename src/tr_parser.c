#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <traceroute.h>

static void TR_badOption(const char * option, int idx) {
  fprintf(stderr, "Bad option: `%s' (argc %d)\n", option, idx);
  exit(TR_FAILURE);
}

static void TR_missingArg(const char * name) {
  fprintf(stderr, "Specify \"%s\" missing argument.\n", name);
  exit(TR_FAILURE);
}

static void TR_missingOptionArg(const char * option, int idx, char * usage) {
  fprintf(
      stderr,
      "Option `%s' (argc %d) requires an argument: `%s'\n",
      option,
      idx,
      usage
  );
  exit(TR_FAILURE);
}

static void TR_extraArg(const char * arg, size_t pos, int idx) {
  fprintf(stderr, "Extra arg `%s' (position %zu, argc %d)\n", arg, pos, idx);
  exit(TR_FAILURE);
}

static void
TR_invalidArg(const char * name, const char * arg, size_t pos, int index) {
  fprintf(
      stderr,
      "Cannot handle \"%s\" cmdline arg `%s' on position %zu (argc %d)\n",
      name,
      arg,
      pos,
      index
  );
  exit(TR_FAILURE);
}

static void
TR_invalidOptionArg(const char * option, const char * arg, int index) {
  fprintf(
      stderr,
      "Cannot handle \"%s\" option with arg `%s' (argc %d)\n",
      option,
      arg,
      index
  );
  exit(TR_FAILURE);
}

int TR_dnsLookup(const char * host, in_addr_t * address) {
  // using getaddrinfo to resolve the host
  struct addrinfo * result;
  struct addrinfo hints = {
      .ai_family = AF_INET,
      .ai_socktype = SOCK_DGRAM,
  };
  if (getaddrinfo(host, NULL, &hints, &result))
    return 1;
  *address = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
  freeaddrinfo(result);
  return 0;
};

TR_Options TR_parseArgs(int argc, const char * const * argv) {
  size_t arg_cnt = 0;
  TR_Options options = {
      .global_opts = argc == 1 ? TR_HELP : 0,
      .packetLen = TR_PACKET_LEN_DEFAULT,
      .firstTtl = TR_FIRST_TTL_DEFAULT,
      .maxTtl = TR_MAX_TTL_DEFAULT,
      .nQueries = TR_NQUERIES_DEFAULT,
  };

  for (int i = 1; i < argc; ++i) {
    if (*argv[i] != '-' || !argv[i][1]) {
      // arg

      switch (arg_cnt++) {
        case 0:
          // host

          options.dstHost = argv[i];
          if (TR_dnsLookup(argv[i], &options.dstAddress))
            TR_invalidArg("host", argv[i], arg_cnt, i);
          break;
        case 1:
          // packetlen

          char * endptr;
          errno = 0;
          const unsigned long packetLen = strtoul(argv[i], &endptr, 0);
          if (errno == ERANGE || *endptr || packetLen > TR_PACKET_LEN_MAX)
            TR_invalidArg("packetlen", argv[i], arg_cnt, i);
          options.packetLen = packetLen;
          break;
        default:
          // extra arg

          TR_extraArg(argv[i], arg_cnt, i);
      }
      continue;
    }
    // option

    if (argv[i][1] == '-') {
      // long option

      if (!strcmp(argv[i] + 2, "help")) {
        options.global_opts |= TR_HELP;
        return options;
      }
      if (!strncmp(argv[i] + 2, "first", 5)) {
        char * endptr;
        errno = 0;
        if (argv[i][7] != '=')
          TR_missingOptionArg("--first", i, "--first=first_ttl");
        const unsigned long firstTtl = strtoul(argv[i] + 8, &endptr, 0);
        if (!argv[i][8] || errno == ERANGE || *endptr ||
            firstTtl > TR_FIRST_TTL_MAX)
          TR_invalidOptionArg("--first", argv[i], i);
        options.firstTtl = firstTtl;
      } else if (!strncmp(argv[i] + 2, "max-hops", 8)) {
        char * endptr;
        errno = 0;
        if (argv[i][10] != '=')
          TR_missingOptionArg("--max", i, "--max=max_ttl");
        const unsigned long maxTtl = strtoul(argv[i] + 11, &endptr, 0);
        if (!argv[i][6] || errno == ERANGE || *endptr ||
            maxTtl > TR_MAX_TTL_MAX)
          TR_invalidOptionArg("--max", argv[i], i);
        options.maxTtl = maxTtl;
      } else
        // unknown option
        TR_badOption(argv[i], i);

    } else {
      // short option(s)

      for (const char * c = argv[i] + 1; *c; ++c) {
        char * endptr;
        errno = 0;

        switch (*c) {
          case 'f':
            if (i + 1 == argc)
              TR_missingOptionArg("-f", i, "-f first_ttl");
            const unsigned long firstTtl = strtoul(argv[++i], &endptr, 0);
            if (!*argv[i] || errno == ERANGE || *endptr ||
                firstTtl > TR_FIRST_TTL_MAX)
              TR_invalidOptionArg("-f", argv[i], i);
            options.firstTtl = firstTtl;
            break;

          case 'm':
            if (i + 1 == argc)
              TR_missingOptionArg("-m", i, "-m max_ttl");
            const unsigned long maxTtl = strtoul(argv[++i], &endptr, 0);
            if (!*argv[i] || errno == ERANGE || *endptr ||
                maxTtl > TR_MAX_TTL_MAX)
              TR_invalidOptionArg("-m", argv[i], i);
            options.maxTtl = maxTtl;
            break;

          case 'q':
            if (i + 1 == argc)
              TR_missingOptionArg("-q", i, "-q nqueries");
            const unsigned long nQueries = strtoul(argv[++i], &endptr, 0);
            if (!*argv[i] || errno == ERANGE || *endptr ||
                nQueries > TR_NQUERIES_MAX)
              TR_invalidOptionArg("-q", argv[i], i);
            options.nQueries = nQueries;
            break;

          default:
            // unknown option

            TR_badOption((char[]){'-', *c, 0}, i);
        }
      }
    }
  }
  if (!(options.global_opts & TR_HELP) && !arg_cnt)
    TR_missingArg("host");
  return options;
}
