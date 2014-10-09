/* The memory map.
 * Define the location and size of the read only (rom) and
 * read/write (ram) memory areas.
 * See sections.ld for how output sections are assigned to
 * these memory regions.
 */
MEMORY
{
  rom (rx) : ORIGIN = 0x00000000, LENGTH = 64M
  ram (! rx) : ORIGIN = 0x48000000, LENGTH = 32M
}

INCLUDE romram.ld
