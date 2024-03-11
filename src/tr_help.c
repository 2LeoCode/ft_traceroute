#include <stdio.h>

int TR_help(void) {
  printf("Usage:\n"
         "  traceroute [ -46dFITnreAUDV ] [ -f first_ttl ] [ -g gate,... ] [ "
         "-i device ] [ -m max_ttl ] [ -N squeries ] [ -p port ] [ -t tos ] [ "
         "-l flow_label ] [ -w MAX,HERE,NEAR ] [ -q nqueries ] [ -s src_addr ] "
         "[ -z sendwait ] [ --fwmark=num ] host [ packetlen ]\n"
         "Options:\n"
         "  -f first_ttl  --first=first_ttl\n"
         "                              Start from the first_ttl hop (instead "
         "from 1)\n"
         "  -m max_ttl  --max-hops=max_ttl\n"
         "                              Set the max number of hops (max TTL to "
         "be\n"
         "                              reached). Default is 30\n"
         "  -q nqueries  --queries=nqueries\n"
         "                              Set the number of probes per each hop. "
         "Default is\n"
         "                              3\n"
         "  --help                      Read this help and exit\n"
         "\n"
         "Arguments:\n"
         "+     host          The host to traceroute to\n"
         "      packetlen     The full packet length (default is the length of "
         "an IP\n"
         "                    header plus 40). Can be ignored or increased to "
         "a minimal\n"
         "                    allowed value\n");
  return 0;
}
