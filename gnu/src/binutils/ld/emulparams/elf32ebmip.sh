# RICH: EMBEDDED=yes
. ${srcdir}/emulparams/elf32bmip.sh

unset DATA_ADDR
SHLIB_TEXT_START_ADDR=0

# Place .got.plt as close to .plt as possible so that the former can be
# referred to from the latter with the microMIPS ADDIUPC instruction
# that only has a span of +/-16MB.
PLT_NEXT_DATA=
INITIAL_READWRITE_SECTIONS=$OTHER_READWRITE_SECTIONS
unset OTHER_READWRITE_SECTIONS
