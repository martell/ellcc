__asm__("\
.text \n\
.global _start \n\
.type _start,%function \n\
_start: \n\
	mov fp, #0 \n\
	mov lr, #0 \n\
	mov a1, sp \n\
	bic sp, sp, #0xF \n\
	bl __cstart \n\
");
