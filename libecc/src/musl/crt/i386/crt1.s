.weak _init
.weak _fini
.text
.global _start
_start:
	xor %ebp,%ebp
	pop %ecx
	mov %esp,%eax
	and $-16,%esp
	push %esp
	push %esp
	push %edx
	push $_fini
	push $_init
	push %eax
	push %ecx
	push $main
	call __libc_start_main
1:	jmp 1b

	.type	__dso_handle,@object
	.bss
	.globl	__dso_handle
	.align	4
__dso_handle:
	.long	0
	.size	__dso_handle, 4
