/** File definitions.
 */
#ifndef _file_h_
#define _file_h_

#include <sys/types.h>

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
  int (*read)(struct file *, off_t *, struct uio *);
  int (*write)(struct file *, off_t *, struct uio *);
  int (*ioctl)(struct file *, unsigned long, void *);
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

typedef struct file
{
  lock_t lock;                  // The lock protecting the file.
  off_t offset;                 // The current file offset.
  unsigned references;          // The number of references to this file.
  const fileops_t *fileops;     // Operations on a file.
  filetype_t type;              // Type of the file.
} file_t;

// Some default fileops.
int fbadop_read(file_t *, off_t *, struct uio *);
int fbadop_write(file_t *, off_t *, struct uio *);
int fbadop_ioctl(file_t *, unsigned long, void *);
int fbadop_fcntl(file_t *, unsigned int, void *);
int fbadop_stat(file_t *, struct stat *);
int fbadop_poll(file_t *, int);
int fbadop_close(file_t *);

int fnullop_read(file_t *, off_t *, struct uio *);
int fnullop_write(file_t *, off_t *, struct uio *);
int fnullop_ioctl(file_t *, unsigned long, void *);
int fnullop_fcntl(file_t *, unsigned int, void *);
int fnullop_stat(file_t *, struct stat *);
int fnullop_poll(file_t *, int);
int fnullop_close(file_t *);

typedef struct fd_set *fdset_t;

/** Release a set of file descriptors.
 */
void __elk_fdset_release(fdset_t *fdset);

/** Clone or copy the fdset.
 */
int __elk_fdset_clone(fdset_t *fdset, int clone);

/** Add a file descriptor to a set.
 */
int __elk_fdset_add(fdset_t *fdset, filetype_t type, const fileops_t *fileops);

#endif
