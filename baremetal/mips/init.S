    .text
    .global _start
_start:
    lui     $sp, %hi(__mem_top)
    addiu   $sp, $sp, %lo(__mem_top)
    lui     $t9, %hi(_startup)
    addiu   $t9, %lo(_startup)
    jalr    $t9
    nop
    b       .

