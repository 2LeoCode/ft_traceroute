#ifndef MISC_H
# define MISC_H

# include <stddef.h>


void *
ft_memcpy(void * dst, const void * src, size_t n);

size_t
ft_strlen(const char * s);

long
ft_strtol(const char * nptr, char ** endptr, int base);

#endif
