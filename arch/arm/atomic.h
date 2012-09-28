#ifndef ARM_ATOMIC_H_
#define ARM_ATOMIC_H_

typedef struct {
	u32 counter;
} atomic_t;

#define atomic_set(mem,v) ((mem)->counter = (v))

#define atomic_get(v)	((v)->counter)

static inline unsigned int atomic_add_return(int i, atomic_t *v)
{
	unsigned long tmp;
	unsigned int result;

	__asm__ __volatile__("@ atomic_add_return\n"
"1:	ldrex	%0, [%3]\n"
"	add	%0, %0, %4\n"
"	strex	%1, %0, [%3]\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (*v)
        : "r" (v), "Ir" (i)
	: "cc");

	return result;
}

static inline unsigned int atomic_sub_return(int i, atomic_t *v)
{
	unsigned long tmp;
	int result;

	__asm__ __volatile__("@ atomic_sub_return\n"
"1:	ldrex	%0, [%3]\n"
"	sub	%0, %0, %4\n"
"	strex	%1, %0, [%3]\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (*v)
	: "r" (v), "Ir" (i)
	: "cc");

	return result;
}

#define atomic_inc(v) (atomic_add_return(1, v))
#define atomic_dec(v) (atomic_sub_return(1, v))

#endif
