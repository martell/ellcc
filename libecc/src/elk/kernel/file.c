/** File handling.
 */
#include <pthread.h>
#include "config.h"
#include "kernel.h"
#include "file.h"

typedef struct fd
{
  pthread_mutex_t mutex;        // The mutex protecting the file descriptor.
  unsigned refcnt;              // The number of references to this descriptor.
  file_t file;                  // The file accessed by the file descriptor.
} fd_t;

/** Release a file.
 */
static void file_release(file_t *file)
{
  if (file == NULL || *file == NULL) {
    return;
  }

  pthread_mutex_lock(&(*file)->mutex);
  if (--(*file)->refcnt == 0) {
    // Release the file.
    pthread_mutex_unlock(&(*file)->mutex);
    free(*file);
    *file = NULL;
    return;
  }
  pthread_mutex_unlock(&(*file)->mutex);
}

/** Add a reference to a file.
 */
static int file_reference(file_t file)
{
  if (file == NULL) {
    return 0;
  }

  pthread_mutex_lock(&file->mutex);
  int s = 0;
  if (++file->refcnt == 0) {
    // Too many references.
    --file->refcnt;
    s = -EAGAIN;
  }

  pthread_mutex_unlock(&file->mutex);
  return s;
}

/** Release a file descriptor.
 */
static void fd_release(fd_t **fd)
{
  if (fd == NULL || *fd == NULL) {
    return;
  }

  pthread_mutex_lock(&(*fd)->mutex);
  if (--(*fd)->refcnt == 0) {
    file_release(&(*fd)->file);         // Release the file.
    pthread_mutex_unlock(&(*fd)->mutex);
    free(*fd);
    *fd = NULL;
    return;
  }
  pthread_mutex_unlock(&(*fd)->mutex);
}

/** Add a reference to a file descriptor.
 */
static int fd_reference(fd_t *fd)
{
  if (fd == NULL) {
    return 0;
  }

  pthread_mutex_lock(&fd->mutex);
  int s = 0;
  if (++fd->refcnt == 0) {
    // Too many references.
    --fd->refcnt;
    s = -EAGAIN;
  } else {
    s = file_reference(fd->file);
    if (s < 0) {
      --fd->refcnt;
    }
  }

  pthread_mutex_unlock(&fd->mutex);
  return s;
}

/** Add a reference to a set of file descriptors.
 */
static int __elk_fdset_reference(fdset_t *fdset)
{
  pthread_mutex_lock(&fdset->mutex);

  int s = 0;
  if (++fdset->refcnt == 0) {
    // Too many references.
    --fdset->refcnt;
    s = -EAGAIN;
  } else {
    for (int i = 0; i < fdset->count; ++i) {
      s = fd_reference(fdset->fds[i]);
      if (s >= 0) {
        continue;
      }

      // Undo previous references.
      for (int j = 0; j < i; ++j) {
        fd_release(&fdset->fds[i]);
      }
      break;
    }
  }

  pthread_mutex_unlock(&fdset->mutex);
  return s;
}

/** Release a set of file descriptors.
 */
void __elk_fdset_release(fdset_t *fdset)
{
  pthread_mutex_lock(&fdset->mutex);

  for (int i = 0; i < fdset->count; ++i) {
    fd_release(&fdset->fds[i]);
  }

  if (--fdset->refcnt == 0) {
    // This set is done being used.
    free(fdset->fds);
    fdset->fds = NULL;
    fdset->count = 0;
  }

  pthread_mutex_unlock(&fdset->mutex);
}

/** Clone or copy the fdset.
 */
int __elk_fdset_clone(fdset_t *fdset, int clone)
{
  if (clone) {
    // The parent and child are sharing the set.

    return __elk_fdset_reference(fdset);
  }

  // Make a copy of the fdset.
  // RICH: fdset_t orig = *fdset;
  return -ENOSYS;
}

/** Create a new file.
 */
static int newfile(file_t *res,
                   filetype_t type, const fileops_t *fileops, void *data)
{
  file_t file = malloc(sizeof(*file));
  if (file == NULL) {
    return -ENOMEM;
  }

  pthread_mutex_init(&file->mutex, NULL);
  file->refcnt = 0;
  file->f_offset = 0;
  file->type = type;
  file->fileops = fileops;
  file->data = data;
  *res = file;
  return 0;
}

/** Get an available entry in an fdset.
 */
static int fdset_grow(fdset_t *fdset)
{
  int s;
  if (fdset->fds == NULL) {
    fdset->fds = malloc(INITFDS * sizeof(fd_t *));
    if (fdset->fds == NULL) {
      return -ENOMEM;
    }

    fdset->refcnt = 1;
    fdset->count = INITFDS;
    for (int i = 0; i < INITFDS; ++i) {
      fdset->fds[i] = NULL;
    }

    s = 0;      // Allocate the first file descriptor.
  } else {
    for (s = 0; s < fdset->count; ++s) {
      if (fdset->fds[s] == NULL) {
        break;
      }
    }

    if (s == fdset->count) {
      // No open slot found, double the size of the fd array.
      fd_t **newfds = realloc(fdset->fds,
                              (s * FDMULTIPLIER) * sizeof(fd_t *));
      if (newfds == NULL) {
        return -ENOMEM;
      }

      for (int i = s; i < s * 2; ++i) {
        newfds[i] = NULL;
      }

      fdset->fds = newfds;
      fdset->count = s * FDMULTIPLIER;
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
    return -ENOMEM;
  }

  pthread_mutex_init(&fd->mutex, NULL);
  fd->refcnt = 0;
  fd->file = NULL;
  *res = fd;
  return 0;
}

/** Allocate a new file descriptor.
 */
static int fd_allocate(fdset_t *fdset)
{
  int fd = fdset_grow(fdset);
  if (fd < 0) {
    return fd;
  }

  int s = newfd(&fdset->fds[fd]);
  if (s < 0) {
    return s;
  }

  return fd;
}

/** Create a file descriptor and add it to a set.
 */
static int fdset_add(fdset_t *fdset, file_t file)
{
  int fd = fd_allocate(fdset);
  if (fd < 0) {
    return fd;
  }

  fdset->fds[fd]->file = file;

  for (int i = 0; i < fdset->refcnt; ++i) {
    int s = fd_reference(fdset->fds[fd]);
    if (s >= 0) {
      continue;
    }

    // An error occured adding the reference.
    free(fdset->fds[fd]);
    fdset->fds[fd] = NULL;
    return s;
  }

  return fd;
}

/** Add a file descriptor to a set.
 */
int __elk_fdset_add(fdset_t *fdset,
                    filetype_t type, const fileops_t *fileops, void *data)
{
  file_t file;
  int s = newfile(&file, type, fileops, data);
  if (s < 0) {
    return s;
  }

  s = fdset_add(fdset, file);
  if (s < 0) {
    free(file);
  }

  return s;
}

/** Dup a file descriptor in a set.
 */
int __elk_fdset_dup(fdset_t *fdset, int fd)
{
  if (fd >= fdset->count || fdset->fds[fd] == NULL) {
    return -EBADF;
  }

  return fdset_add(fdset, fdset->fds[fd]->file);
}

/** Remove a file descriptor from a set.
 */
int __elk_fdset_remove(fdset_t *fdset, int fd)
{
  if (fd >= fdset->count || fdset->fds[fd] == NULL) {
    return -EBADF;
  }

  fd_release(&fdset->fds[fd]);
  return 0;
}

/** Read from a file descriptor.
 */
size_t __elk_fdset_read(fdset_t *fdset, int fd, struct uio *uio)
{
  if (fd >= fdset->count || fdset->fds[fd] == NULL) {
    return -EBADF;
  }

  file_t file = fdset->fds[fd]->file;
  return file->fileops->read(file, &file->f_offset, uio);
}

/** Write to a file descriptor.
 */
size_t __elk_fdset_write(fdset_t *fdset, int fd, struct uio *uio)
{
  if (fd >= fdset->count || fdset->fds[fd] == NULL) {
    return -EBADF;
  }

  file_t file = fdset->fds[fd]->file;
  return file->fileops->write(file, &file->f_offset, uio);
}

/** Do an ioctl on a file descriptor.
 */
int __elk_fdset_ioctl(fdset_t *fdset, int fd, int cmd, void *arg)
{
  if (fd >= fdset->count || fdset->fds[fd] == NULL) {
    return -EBADF;
  }

  file_t file = fdset->fds[fd]->file;
  return file->fileops->ioctl(file, cmd, arg);
}


ssize_t fbadop_read(file_t file, off_t *off, struct uio *uiop)
{
  return -ENOSYS;
}

ssize_t fbadop_write(file_t file, off_t *off, struct uio *uiop)
{
  return -ENOSYS;
}

int fbadop_ioctl(file_t file, unsigned int cmd, void *arg)
{
  return -ENOSYS;
}

int fbadop_fcntl(file_t file, unsigned int cmd, void *arg)
{
  return -ENOSYS;
}

int fbadop_stat(file_t file, struct stat *buf)
{
  return -ENOSYS;
}

int fbadop_poll(file_t file, int events)
{
  return -ENOSYS;
}

int fbadop_close(file_t file)
{
  return -ENOSYS;
}

ssize_t fnullop_read(file_t file, off_t *off, struct uio *uiop)
{
  return 0;
}

ssize_t fnullop_write(file_t file, off_t *off, struct uio *uiop)
{
  return 0;
}

int fnullop_ioctl(file_t file, unsigned int cmd, void *arg)
{
  return 0;
}

int fnullop_fcntl(file_t file, unsigned int cmd, void *arg)
{
  return 0;
}

int fnullop_stat(file_t file, struct stat *buf)
{
  return 0;
}

int fnullop_poll(file_t file, int events)
{
  return 0;
}

int fnullop_close(file_t file)
{
  return 0;
}
