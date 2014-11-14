/** File definitions.
 */
#ifndef _file_h_
#define _file_h_

#include <sys/types.h>
#include "kernel.h"

typedef struct uio
{
  int iovcnt;
  const struct iovec *iov;
} uio_t;

struct stat;
struct file;

/** Operations on a file.
 */
typedef struct fileops
{
  ssize_t (*read)(struct file *, off_t *, struct uio *);
  ssize_t (*write)(struct file *, off_t *, struct uio *);
  int (*ioctl)(struct file *, unsigned int, void *);
  int (*fcntl)(struct file *, unsigned int, void *);
  int (*poll)(struct file *, int);
  int (*stat)(struct file *, struct stat *);
  int (*close)(struct file *);
} fileops_t;

typedef enum
{
  FTYPE_NONE,                   // Undefined.
  FTYPE_FILE,                   // File.
  FTYPE_SOCKET,                 // Communications endpoint.
  FTYPE_PIPE,                   // Pipe.
  FTYPE_MISC,                   // User defined.

  FTYPE_END                     // To get the number of types.
} filetype_t;

#if defined(DEFINE_FILETYPE_STRINGS)
static const char *filetype_names[FTYPE_END] =
{
  [FTYPE_NONE] = "none",
  [FTYPE_FILE] = "file",
  [FTYPE_SOCKET] = "socket",
  [FTYPE_PIPE] = "pipe",
  [FTYPE_MISC] = "misc",
};
#endif

struct file;

// Some default fileops.
ssize_t fbadop_read(struct file *, off_t *, struct uio *);
ssize_t fbadop_write(struct file *, off_t *, struct uio *);
int fbadop_ioctl(struct file *, unsigned int, void *);
int fbadop_fcntl(struct file *, unsigned int, void *);
int fbadop_stat(struct file *, struct stat *);
int fbadop_poll(struct file *, int);
int fbadop_close(struct file *);

ssize_t fnullop_read(struct file *, off_t *, struct uio *);
ssize_t fnullop_write(struct file *, off_t *, struct uio *);
int fnullop_ioctl(struct file *, unsigned int, void *);
int fnullop_fcntl(struct file *, unsigned int, void *);
int fnullop_stat(struct file *, struct stat *);
int fnullop_poll(struct file *, int);
int fnullop_close(struct file *);

// A set of file descriptors.
typedef struct fdset
{
  pthread_mutex_t mutex;        // The mutex protecting the set.
  unsigned references;          // The number of references to this set.
  unsigned count;               // Number of file descriptors in the set.
  struct fd **fds;              // The file descriptor nodes.
} fdset_t;

/** Release a set of file descriptors.
 */
void __elk_fdset_release(fdset_t *fdset);

/** Clone or copy the fdset.
 */
int __elk_fdset_clone(fdset_t *fdset, int clone);

/** Add a file descriptor to a set.
 */
int __elk_fdset_add(fdset_t *fdset,
                    filetype_t type, const fileops_t *fileops, void *data);

/** Remove a file descriptor from a set.
 */
int __elk_fdset_remove(fdset_t *fdset, int fd);

/** Dup a file descriptor in a set.
 */
int __elk_fdset_dup(fdset_t *fdset, int fd);

/** Write to a file descriptor.
 */
size_t __elk_fdset_write(fdset_t *fdset, int fd, struct uio *uio);

/** Read from a file descriptor.
 */
size_t __elk_fdset_read(fdset_t *fdset, int fd, struct uio *uio);

/** Do an ioctl on a file descriptor.
 */
int __elk_fdset_ioctl(fdset_t *fdset, int fd, int cmd, void *arg);

#endif
