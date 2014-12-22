#if VM_COMMANDS
#include <stdio.h>
#include "command.h"

/** Display a virtual memory map.
 */
static void display_map(pid_t pid, vm_map_t map)
{
  printf("VM map for pid %d\n", pid);
  printf("reference count %d\n", map->refcnt);
  printf("size            %zd bytes\n", map->total);
  printf("Segments:\n");
  printf("%10.10s "
         "%10.10s %10.10s %10.10s %10.10s "
         "%10.10s %10.10s %10.10s %10.10s\n",
         "ADDR",
         "VADDR", "SIZE", "PADDR", "FLAGS",
         "NEXT", "PREV", "SH_NEXT", "SH_PREV");
  struct seg *seg = &map->head;
  do {
    printf("%8p 0x%08lx %10zd 0x%08lx 0x%08x %8p %8p %8p %8p\n",
           seg,
           seg->addr, seg->size, seg->phys, seg->flags,
           seg->next, seg->prev, seg->sh_next, seg->sh_prev);
    seg = seg->next;
  } while(seg && seg != &map->head);
}

static int vmCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("show process virtual memory.\n");
    return COMMAND_OK;
  }

  int pid = 0;
  while (argc > 1) {
    const char *p = argv[1];
    if (*p++ != '-') {
      break;
    }

    while (*p) {
      switch (*p) {
      case 'p':
        pid = 1;
        break;
      default:
        fprintf(stderr, "unknown option character '%c'\n", *p);
        return COMMAND_ERROR;
      }
      ++p;
    }

    --argc;
    ++argv;
  }

  if (argc != 1) {
    fprintf(stderr, "invalid argument \"%s\"\n", argv[1]);
    return COMMAND_ERROR;
  }

  display_map(pid, getmap(pid));
  return COMMAND_OK;
}

/** Create a section heading for the help command.
 */
static int sectionCommand(int argc, char **argv)
{
  if (argc <= 0 ) {
    printf("Virtual Memory Commands:\n");
  }
  return COMMAND_OK;
}

#endif  // VM_COMMANDS

C_CONSTRUCTOR()
{
#if VM_COMMANDS
  command_insert(NULL, sectionCommand);
  command_insert("vm", vmCommand);
#endif
}

