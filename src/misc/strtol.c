#include <misc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <limits.h>
#include <errno.h>

#define AddRangeCheck(a, b, RANGE) ({ \
  if (a > 0 && b > RANGE ## _MAX - a) { \
    errno = ERANGE; \
    return RANGE ## _MAX; \
  } \
  if (a < 0 && b < RANGE ## _MIN - a) { \
    errno = ERANGE; \
    return RANGE ## _MIN; \
  } \
  a + b; \
})

#define MulRangeCheck(a, b, RANGE) ({ \
  if (a > 0 && b > RANGE ## _MAX / a) { \
    errno = ERANGE; \
    return RANGE ## _MAX; \
  } \
  if (a < 0 && b < RANGE ## _MIN / a) { \
    errno = ERANGE; \
    return RANGE ## _MIN; \
  } \
  a * b; \
})

long
ft_strtol(const char * nptr, char ** endptr, int base) {
  if (base < 0 || base == 1 || base > 36) {
    errno = EINVAL;
    return 0;
  }
  if (base == 0)
    base = 10;

  long result = 0;
  int sign = 1;

  if (*nptr == '-' || *nptr == '+') {
    sign = 1 - 2 * (*nptr == '-');
    ++nptr;
  }

  if (base == 16 && *nptr == '0' && (nptr[1] == 'x' || nptr[1] == 'X'))
    nptr += 2;

  for (char c = *nptr; c; c = *++nptr) {
    int digit;

    if (c >= '0' && c <= '9')
      digit = c - '0';
    else if (c >= 'a' && c <= 'z')
      digit = c - 'a' + 10;
    else if (c >= 'A' && c <= 'Z')
      digit = c - 'A' + 10;
    else
      break;
    if (digit >= base)
      break;
    result = AddRangeCheck(result, base, LONG);
    result = MulRangeCheck(result, sign * digit, LONG);
  }
  if (endptr)
    *endptr = (char *)nptr;
  return result;
}
