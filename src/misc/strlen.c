#include <stddef.h>

size_t
ft_strlen(const char * s) {
  char * end = s;
  while (*end)
    ++end;
  return end - s;
}
