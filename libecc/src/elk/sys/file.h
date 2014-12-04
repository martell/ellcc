/** File definitions.
 */
#ifndef _file_h_
#define _file_h_

#include <sys/types.h>
#include "config.h"
#include "kernel.h"
#include "uio.h"

struct stat;
struct file;

/* Kernel encoding of open mode; separate read and write bits that are
 * independently testable.
 */
#define FREAD           0x00000001
#define FWRITE          0x00000002

// Convert from open() flags to/from f_flags; convert O_RD/WR to FREAD/FWRITE.
#define FFLAGS(oflags)  ((oflags) + 1)
#define OFLAGS(fflags)  ((fflags) - 1)

typedef struct file
{
  off_t f_offset;               // The current file offset.
  int f_flags;                  // Open flags.
  unsigned f_count;             // Reference count.
  const struct vnode *f_vnode;  // The file's vnode.
} *file_t;

typedef struct fdset *fdset_t;

#if ELK_NAMESPACE
#define fdset_release __elk_fdset_release
#define fdset_clone __elk_fdset_clone
#define fdset_add __elk_fdset_add
#define fdset_addfd __elk_fdset_addfd
#define fdset_remove __elk_fdset_remove
#define fdset_dup __elk_fdset_dup
#define fdset_getfile __elk_fdset_getfile
#define fdset_getdup __elk_fdset_getdup
#define fdset_setfile __elk_fdset_setfile
#define fdset_new __elk_fdset_new
#endif

/** Release a set of file descriptors.
 */
void fdset_release(fdset_t fdset);

/** Clone or copy the fdset.
 */
int fdset_clone(fdset_t *fdset, int clone);

/** Add a file descriptor to a set.
 */
int fdset_add(fdset_t fdset, file_t file);

/** Add a file specific file descriptor to a set.
 */
int fdset_addfd(fdset_t fdset, int fd, file_t file);

/** Remove a file descriptor from a set.
 */
int fdset_remove(fdset_t fdset, int fd);

/** Dup a file descriptor in a set.
 */
int fdset_dup(fdset_t fdset, int fd);

/** Get a file pointer corresponding to a file descriptor.
 */
int fdset_getfile(fdset_t fdset, int fd, file_t *filep);

/** Get a file pointer corresponding to a file descriptor.
 * If free, find the first available free file descriptor.
 */
int fdset_getdup(fdset_t fdset, int fd, file_t *filep, int free);

/** Set a file pointer corresponding to a file descriptor.
 */
int fdset_setfile(fdset_t fdset, int fd, file_t file);

/** Create a new fdset.
 */
int fdset_new(fdset_t *fdset);

#endif
