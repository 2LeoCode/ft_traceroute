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
      .method = TR_UDP,
      .packetLen = TR_PACKET_LEN_DEFAULT,
      .firstTtl = TR_FIRST_TTL_DEFAULT,
      .maxTtl = TR_MAX_TTL_DEFAULT,
      .nQueries = TR_NQUERIES_DEFAULT,
      .wait = {
          .max = TR_WAIT_MAX_DEFAULT,
          .here = TR_WAIT_HERE_DEFAULT,
          .near = TR_WAIT_NEAR_DEFAULT,
      }};

  char * endptr;
  errno = 0;

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
        if (argv[i][7] != '=' || !argv[i][8])
          TR_missingOptionArg("--first", i, "--first=first_ttl");
        const unsigned long firstTtl = strtoul(argv[i] + 8, &endptr, 0);
        if (errno == ERANGE || *endptr || firstTtl > TR_FIRST_TTL_MAX)
          TR_invalidOptionArg("--first", argv[i] + 8, i);
        options.firstTtl = firstTtl;
      } else if (!strncmp(argv[i] + 2, "max-hops", 8)) {
        if (argv[i][10] != '=' || !argv[i][11])
          TR_missingOptionArg("--max-hops", i, "--max-hops=max_ttl");
        const unsigned long maxTtl = strtoul(argv[i] + 11, &endptr, 0);
        if (errno == ERANGE || *endptr || maxTtl > TR_MAX_TTL_MAX)
          TR_invalidOptionArg("--max-hops", argv[i] + 11, i);
        options.maxTtl = maxTtl;
      } else if (!strncmp(argv[i] + 2, "wait", 4)) {
        if (argv[i][6] != '=')
          TR_missingOptionArg("--wait", i, "--wait=wait_time");
        options.wait.max = strtod(argv[i] + 7, &endptr);
        if (!argv[i][6] || errno == ERANGE)
          TR_invalidOptionArg("--wait", argv[i] + 7, i);
        if (!*endptr) {
          options.wait.here = options.wait.near = 0;
          continue;
        }
        if (*endptr != ',' || !endptr[1])
          TR_invalidOptionArg("--wait", argv[i] + 7, i);
        options.wait.here = strtod(endptr + 1, &endptr);
        if (errno == ERANGE || (*endptr && (*endptr != ',' || !endptr[1])))
          TR_invalidOptionArg("--wait", argv[i] + 7, i);
        if (!*endptr)
          continue;
        options.wait.near = strtod(endptr + 1, &endptr);
        if (*endptr)
          TR_invalidOptionArg("--wait", argv[i] + 7, i);
      } else
        // unknown option
        TR_badOption(argv[i], i);

    } else {
      // short option(s)

      for (const char * c = argv[i] + 1; *c; ++c) {

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

          case 'w':
            if (i + 1 == argc)
              TR_missingOptionArg("-w", i, "-w wait_time");
            options.wait.max = strtod(argv[++i], &endptr);
            if (!*argv[i] || errno == ERANGE)
              TR_invalidOptionArg("-w", argv[i], i);
            if (!*endptr) {
              options.wait.here = options.wait.near = 0;
              continue;
            }
            if (*endptr != ',' || !endptr[1])
              TR_invalidOptionArg("-w", argv[i], i);
            options.wait.here = strtod(endptr + 1, &endptr);
            if (errno == ERANGE || (*endptr && (*endptr != ',' || !endptr[1])))
              TR_invalidOptionArg("-w", argv[i], i);
            if (!*endptr)
              continue;
            options.wait.near = strtod(endptr + 1, &endptr);
            if (errno == ERANGE || *endptr)
              TR_invalidOptionArg("-w", argv[i], i);
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
