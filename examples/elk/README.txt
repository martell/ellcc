An example of building ELK.
The file "elkconfig.LD" contains the configuration parameters for the
build. The definitions of FLASH and MMU determine how ELK should run:

FLASH   MMU
  X                     Run ELK from flash.
  X     X               Run ELK from flash, enable virtual memory.
  X                     Run ELK from RAM.
  X     X               Run ELK from RAM, enable virtual memory.

Running ELK from flash makes allows it to act like a boot loader
similar to U-Boot. Running from RAM makes ELK look more like a
Linux kernel.

Currently supported configuration(s):
Linux:
  None
ELK:
  arm-elk-engeabi
