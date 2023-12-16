#include <misc.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

# define TR_SUCCESS 0
# define TR_FAILURE 2

typedef struct tr_data tr_data;
typedef struct tr_options tr_options;

enum tr_global_options {
  TR_HELP = 1,
};

enum tr_module_options {
  TR_MODULE_DEFAULT = 0,
};

struct tr_data {
  int socket;
};

struct tr_options {
  int global_options;
  int module_options;
  size_t packetlen;
};

int tr_bad_option(int index, const char * option) {
  fprintf(stderr, "Bad option: `%s' (argc %d)\n", option, index);
  return -1;
}

int tr_parse_options(int * argc, const char ** argv) {
  int options = *argc == 1 ? TR_HELP : 0;

  for (int i = 1; i < *argc; ++i) {
    if (*argv[i] != '-')
      continue;
    // option

    if (argv[i][1] == '-') {
      // long option

      if (!ft_strcmp(argv[i] + 2, "help"))
        options |= TR_HELP;
      else {
        // unknown option

        return tr_bad_option(i, argv[i]);
      }
    } else {
      // short option(s)

      for (char * c = argv[i] + 1; *c; ++c) {
        switch (*c) {
          default:
            return tr_bad_option(i, (char []){ '-', *c, 0 });
        }
      }
    }
    // remove option from argv

    ft_memcpy(argv + i, argv + i + 1, (*argc - i) * sizeof(char *));
    --*argc;
    --i;
  }
  return options;
}

int tr_help(void) {
  printf(
    "Usage:\n"
    "  ft_traceroute host [ packetlen ]\n"
  );
  return TR_SUCCESS;
}

int tr_missing_arg(const char * arg) {
  fprintf(stderr, "Specify \"%s\" missing argument.", arg);
  return TR_FAILURE;
}

tr_data tr_init(tr_options * options) {
  tr_data data;

  data.socket = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
}

int tr_main(int argc, const char * const * argv) {
  // Not implemented
  return TR_SUCCESS;
}

int main(const int argc, const char ** const argv) {
  int options = tr_parse_options(&argc, argv);

  if (options == -1)
    return TR_FAILURE;
  if (options & TR_HELP)
    return tr_help();
  if (argc == 1)
    return tr_missing_arg("host");
  return tr_main(argc, argv);
}
