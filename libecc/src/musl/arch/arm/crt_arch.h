__asm__(
".text \n"
".global _start \n"
".type _start,%function \n"
"_start: \n"
"	mov fp, #0 \n"
"	mov lr, #0 \n"
"	mov a1, sp \n"
#if defined(__thumb__)
"       mov a2, sp \n"
"       bic a2, #0xF \n"
"       mov sp, a2 \n"
#else
"	bic sp, sp, #0xF \n"
#endif
"	bl __cstart \n"
);
