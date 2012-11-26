#ifndef CR_BITOPS_H_
#define CR_BITOPS_H_

#define DIV_ROUND_UP(n,d)	(((n) + (d) - 1) / (d))
#define BITS_PER_LONG		(8 * sizeof(long))
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(nr, BITS_PER_LONG)

#define DECLARE_BITMAP(name, bits)		\
	unsigned long name[BITS_TO_LONGS(bits)]

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1)
/* Technically wrong, but this avoids compilation errors on some gcc
   versions. */
#define BITOP_ADDR(x) "=m" (*(volatile long *) (x))
#else
#define BITOP_ADDR(x) "+m" (*(volatile long *) (x))
#endif

#define ADDR				BITOP_ADDR(addr)

#include <arch_bitops.h>

#define for_each_bit(i, bitmask)				\
	for (i = find_next_bit(bitmask, sizeof(bitmask), 0);	\
	     i < sizeof(bitmask);				\
	     i = find_next_bit(bitmask, sizeof(bitmask), i + 1))

#endif /* CR_BITOPS_H_ */
