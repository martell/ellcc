.global _Reset
    .text
    .globl _Reset
_Reset:
    lui     $sp, %hi(stack_top)
    addiu   $sp, $sp, %lo(stack_top)
    lui     $t9, %hi(_startup)
    addiu   $t9, %lo(_startup)
    jalr    $t9
    nop
hang:
    b       hang

