#ifndef ARM_BITOPS_H_
#define ARM_BITOPS_H_

static inline void set_bit(int nr, volatile unsigned long *addr) {
	*addr |= (1 << nr);
}

static inline int test_bit(int nr, volatile const unsigned long *addr) {
	return (*addr & (1 << nr)) ? 1 : 0;
}

static inline void clear_bit(int nr, volatile unsigned long *addr) {
	*addr &= ~(1 << nr);
}

static inline unsigned long __ffs(unsigned long word) {
	int p = 0;

	for (; p < 8*sizeof(word); ++p) {
		if (word & 1) {
			break;
		}

		word <<= 1;
	}

	return p;
}

#endif
