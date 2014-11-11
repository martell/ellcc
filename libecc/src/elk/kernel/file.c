/** File handling.
 */
#include "kernel.h"
#include "file.h"

typedef struct fd
{
  lock_t lock;                  // The lock protecting the file descriptor.
  unsigned references;          // The number of references to this descriptor.
  file_t *file;                 // The file accessed by the file descriptor.
} fd_t;

// A set of file descriptors.
struct fd_set
{
  lock_t lock;                  // The lock protecting the set.
  unsigned references;          // The number of references to this set.
  unsigned count;               // Number of file descriptors in the set.
  fd_t *fds[];                  // The file descriptor nodes.
};

/** Release a file.
 */
static void file_release(file_t *file)
{
  if (file == NULL) {
    return;
  }

  __elk_lock_aquire(&file->lock);
  if (--file->references == 0) {
    // Release the file.
    __elk_lock_release(&file->lock);
    free(file);
    return;
  }
  __elk_lock_release(&file->lock);
}

/** Add a reference to a file.
 */
static int file_reference(file_t *file)
{
  if (file == NULL) {
    return 0;
  }

  __elk_lock_aquire(&file->lock);
  int s = 0;
  if (++file->references == 0) {
    // Too many references.
    --file->references;
    s = -EAGAIN;
  }

  __elk_lock_release(&file->lock);
  return s;
}

/** Release a file descriptor.
 */
static void fd_release(fd_t *fd)
{
  if (fd == NULL) {
    return;
  }

  __elk_lock_aquire(&fd->lock);
  if (--fd->references == 0) {
    file_release(fd->file);      // Release the file.
    __elk_lock_release(&fd->lock);
    free(fd);
    return;
  }
  __elk_lock_release(&fd->lock);
}

/** Add a reference to a file descriptor.
 */
static int fd_reference(fd_t *fd)
{
  if (fd == NULL) {
    return 0;
  }

  __elk_lock_aquire(&fd->lock);
  int s = 0;
  if (++fd->references == 0) {
    // Too many references.
    --fd->references;
    s = -EAGAIN;
  } else {
    s = file_reference(fd->file);
    if (s < 0) {
      --fd->references;
    }
  }

  __elk_lock_release(&fd->lock);
  return s;
}

/** Add a reference to a set of file descriptors.
 */
static int __elk_fdset_reference(fdset_t fdset)
{
  __elk_lock_aquire(&fdset->lock);

  int s = 0;
  if (++fdset->references == 0) {
    // Too many references.
    --fdset->references;
    s = -EAGAIN;
  } else {
    for (int i = 0; i < fdset->count; ++i) {
      s = fd_reference(fdset->fds[i]);
      if (s >= 0) {
        continue;
      }

      // Undo previous references.
      for (int j = 0; j < i; ++j) {
        fd_release(fdset->fds[i]);
      }
      break;
    }
  }

  __elk_lock_release(&fdset->lock);
  return s;
}

/** Release a set of file descriptors.
 */
void __elk_fdset_release(fdset_t *fdset)
{
  if (*fdset == NULL) {
    // No descriptors allocated.
    return;
  }

  __elk_lock_aquire(&(*fdset)->lock);

  if (--(*fdset)->references == 0) {
    // This set is done being used.
    for (int i = 0; i < (*fdset)->count; ++i) {
      fd_release((*fdset)->fds[i]);
    }
    __elk_lock_release(&(*fdset)->lock);
    free(*fdset);
    *fdset = NULL;
    return;
  }

  __elk_lock_release(&(*fdset)->lock);
}

/** Clone or copy the fdset.
 */
int __elk_fdset_clone(fdset_t *fdset, int clone)
{
  if (*fdset == NULL) {
    // No descriptors allocated.
    return 0;
  }

  if (clone) {
    // The parent and child are sharing the set.

    return __elk_fdset_reference(*fdset);
  }

  // Make a copy of the fdset.
  // RICH: fdset_t orig = *fdset;
  return -ENOSYS;
}

/** Create a new file.
 */
static int newfile(file_t **res, filetype_t type, const fileops_t *fileops)
{
  file_t *file = malloc(sizeof(file_t));
  if (file == NULL) {
    return -errno;
  }

  file->lock = (lock_t)LOCK_INITIALIZER;
  file->references = 0;
  file->offset = 0;
  file->type = type;
  file->fileops = fileops;
  *res = file;
  return 0;
}

/** Get an available entry in an fdset.
 */
#define INITFDS 4
static int fdset_grow(fdset_t *fdset)
{
  int s;
  if (*fdset == NULL) {
    fdset_t newset = malloc(sizeof(fdset_t *) + (INITFDS * sizeof(fd_t *)));
    if (newset == NULL) {
      return -errno;
    }

    newset->lock = (lock_t)LOCK_INITIALIZER;
    newset->references = 1;
    newset->count = INITFDS;
    for (int i = 0; i < INITFDS; ++i) {
      newset->fds[i] = NULL;
    }

    *fdset = newset;
    s = 0;
  } else {
    for (s = 0; s < (*fdset)->count; ++s) {
      if ((*fdset)->fds[s] == NULL) {
        break;
      }
    }

    if (s == (*fdset)->count) {
      // No open slot found, double the size of the fd array.
      fdset_t newset = realloc(*fdset,
                               sizeof(fdset_t *) + (s * 2) * sizeof(fd_t *));
      if (newset == NULL) {
        return -errno;
      }

      newset->count = s * 2;

      for (int i = s; i < s * 2; ++i) {
        newset->fds[i] = NULL;
      }

      *fdset = newset;
    }
  }

  return s;
}

/** Create a new file descriptor entry.
 */
static int newfd(fd_t **res)
{
  fd_t *fd = malloc(sizeof(fd_t));
  if (fd == NULL) {
    return -errno;
  }

  fd->lock = (lock_t)LOCK_INITIALIZER;
  fd->references = 0;
  fd->file = NULL;
  *res = fd;
  return 0;
}

/** Create a file descriptor and add it to a set.
 */
static int fdset_add(fdset_t *fdset, file_t *file)
{
  int fd = fdset_grow(fdset);
  if (fd < 0) {
    return fd;
  }

  int s = newfd(&(*fdset)->fds[fd]);
  if (s < 0) {
    return s;
  }

  (*fdset)->fds[fd]->file = file;
  for (int i = 0; i < (*fdset)->references; ++i) {
    s = fd_reference((*fdset)->fds[fd]);
    if (s >= 0) {
      continue;
    }

    free((*fdset)->fds[fd]);
    (*fdset)->fds[fd] = NULL;
    return s;
  }

  return fd;
}

/** Add a file descriptor to a set.
 */
int __elk_fdset_add(fdset_t *fdset, filetype_t type, const fileops_t *fileops)
{
  file_t *file;
  int s = newfile(&file, type, fileops);
  if (s < 0) {
    return s;
  }

  s = fdset_add(fdset, file);
  if (s < 0) {
    free(file);
  }

  return s;
}

/** Remove a file descriptor from a set.
 */
int __elk_fdset_remove(fdset_t *fdset, int fd)
{
  if (fd >= (*fdset)->count || (*fdset)->fds[fd] == NULL) {
    return -EBADF;
  }

  fd_release((*fdset)->fds[fd]);
  (*fdset)->fds[fd] = NULL;
  return 0;
}

int fbadop_read(file_t *file, off_t *off, struct uio *uiop)
{
  return -ENOSYS;
}

int fbadop_write(file_t *file, off_t *off, struct uio *uiop)
{
  return -ENOSYS;
}

int fbadop_ioctl(file_t *file, unsigned long cmd, void *arg)
{
  return -ENOSYS;
}

int fbadop_fcntl(file_t *file, unsigned int cmd, void *arg)
{
  return -ENOSYS;
}

int fbadop_stat(file_t *file, struct stat *buf)
{
  return -ENOSYS;
}

int fbadop_poll(file_t *file, int events)
{
  return -ENOSYS;
}

int fbadop_close(file_t *file)
{
  return -ENOSYS;
}

int fnullop_read(file_t *file, off_t *off, struct uio *uiop)
{
  return 0;
}

int fnullop_write(file_t *file, off_t *off, struct uio *uiop)
{
  return 0;
}

int fnullop_ioctl(file_t *file, unsigned long cmd, void *arg)
{
  return 0;
}

int fnullop_fcntl(file_t *file, unsigned int cmd, void *arg)
{
  return 0;
}

int fnullop_stat(file_t *file, struct stat *buf)
{
  return 0;
}

int fnullop_poll(file_t *file, int events)
{
  return 0;
}

int fnullop_close(file_t *file)
{
  return 0;
}
