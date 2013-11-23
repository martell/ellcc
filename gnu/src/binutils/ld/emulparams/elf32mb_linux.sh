SCRIPT_NAME=elf
OUTPUT_FORMAT="elf32-microblaze"
BIG_OUTPUT_FORMAT="elf32-microblaze"
LITTLE_OUTPUT_FORMAT="elf32-microblazeel"
TEXT_START_ADDR=0x10000000
NONPAGED_TEXT_START_ADDR=0x28
ALIGNMENT=4
MAXPAGESIZE=0x1000
#COMMONPAGESIZE=0x1000
ARCH=microblaze

NOP=0x80000000

TEMPLATE_NAME=elf32
GENERATE_SHLIB_SCRIPT=yes
GENERATE_PIE_SCRIPT=yes
NO_SMALL_DATA=yes
SEPARATE_GOTPLT="SIZEOF (.got.plt) >= 12 ? 12 : 0"

