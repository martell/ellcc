__asm__(
".text \n"
".global " START " \n"
".type " START ",%function \n"
START ": \n"
"	mov fp, #0 \n"
"	mov lr, #0 \n"
"	mov a1, sp \n"
#if defined(__thumb__)
"       mov a2, sp \n"
"       bic a2, #0xF \n"
"       mov sp, a2 \n"
#else
"       bic sp, sp, #0xF \n"
#endif
"	ldr a2, 1f \n"
"2:	add a2, pc \n"
"	bl " START "_c \n"
".weak _DYNAMIC \n"
".hidden _DYNAMIC \n"
"1:	.word _DYNAMIC-2b-8 \n"
);
