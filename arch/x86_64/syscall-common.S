#include "linkage.h"

	.macro syscall0 name, opcode
	ENTRY(\name)
	movl	$\opcode, %eax
	jmp	__syscall_common
	END(\name)
	.endm

#define syscall5 syscall0
#define syscall6 syscall0

	.text
	.align	4

ENTRY(__syscall_common)
	movq	%rcx, %r10
	syscall
	ret
END(__syscall_common)
