#ifndef ARM_PARASITE_H__
#define ARM_PARASITE_H__

static uint32_t get_tls(void) {
	uint32_t res;

	asm (
	     "adr %%r1, kuser_get_tls   \n"
	     "ldr %%r1, [%%r1]          \n"
	     "push { %%r7, %%lr }       \n"
	     "blx %%r1                  \n"
	     "pop { %%r7, %%lr }        \n"
	     "mov %0, %%r0              \n"
	     "b   core_store_tls_done   \n"

	     "kuser_get_tls:            \n"
	     ".word 0xffff0fe0          \n"

	     "core_store_tls_done:      \n"
	     :"=r"(res)
	     : 
	     : "r0", "r1", "memory"
	     );

	return res;
}

#endif
