/** File handling.
 */
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include "config.h"
#include "kernel.h"
#include "file.h"
#include "kmem.h"
#include "vnode.h"

typedef struct fd
{
  file_t file;                  // The file accessed by the file descriptor.
} fd_t;

// A set of file descriptors.
struct fdset
{
  pthread_mutex_t mutex;        // The mutex protecting the file descriptor set.
  unsigned refcnt;              // The file descriptor set reference count.
  unsigned count;               // Number of file descriptors in the set.
  struct fd *fds;               // The file descriptor nodes.
};

/** Initialize a file descriptor.
 */
void init_fd(fd_t *fd)
{
  fd->file = NULL;
}

/** Release a file descriptor.
 */
static void fd_release(fd_t *fd)
{
  // Close the file.
  file_t fp = fd->file;
  if (fp)
    vfs_close(fp);

  fd->file = NULL;
}

/** Add a reference to a file descriptor.
 */
static void fd_reference(fd_t *fd)
{
  file_t fp = fd->file;
  if (fp) {
    vref(fp->f_vnode);
    ++fp->f_count;
  }
}

/** Add a reference to a set of file descriptors.
 */
static void fdset_reference(fdset_t fdset)
{
  for (int i = 0; i < fdset->count; ++i) {
    fd_reference(&fdset->fds[i]);
  }
}

/** Release a set of file descriptors.
 */
void fdset_release(fdset_t fdset)
{
  pthread_mutex_lock(&fdset->mutex);
  ASSERT(fdset->refcnt > 0);
  if (--fdset->refcnt > 0) {
    pthread_mutex_unlock(&fdset->mutex);
    return;
  }

  for (int i = 0; i < fdset->count; ++i) {
    fd_release(&fdset->fds[i]);
  }

  kmem_free(fdset->fds);
  pthread_mutex_unlock(&fdset->mutex);
  kmem_free(fdset);
}

/** Allocate a new file descriptor.
 */
static int fd_allocate(fdset_t fdset, int spec)
{
  int s;
  int initfds = CONFIG_INITFDS;

  if (spec == -1) {
    // Get any file decriptor.
    spec = 0;
  } else if (fdset->fds == NULL) {
    // The set hasn't been created, make a set big enough.
    while (spec > initfds) {
      initfds *= CONFIG_FDMULTIPLIER;
    }
  }

  if (fdset->fds == NULL) {
    fdset->fds = kmem_alloc(initfds * sizeof(fd_t));
    if (fdset->fds == NULL) {
      return -EMFILE;
    }

    fdset->count = initfds;
    for (int i = 0; i < initfds; ++i) {
      init_fd(&fdset->fds[i]);
    }

    // Allocate the requested file descriptor.
    s = spec;
  } else {
    for (s = spec; s < fdset->count; ++s) {
      if (fdset->fds[s].file == NULL) {
        break;
      }
    }

    if (s >= fdset->count) {
      // No open slot found, double the size of the fd array.
      int count = fdset->count;
      while (s >= count) {
        count *= CONFIG_FDMULTIPLIER;
      }

      fd_t *newfds = kmem_realloc(fdset->fds, count * sizeof(fd_t));
      if (newfds == NULL) {
        return -EMFILE;
      }

      for (int i = s; i < count; ++i) {
        init_fd(&newfds[i]);
      }

      fdset->fds = newfds;
      fdset->count = count;
    }
  }

  return s;
}

/** Add a specific file descriptor to a set.
 */
static int addfd(fdset_t fdset, int spec, file_t file)
{
  int fd = fd_allocate(fdset, spec);
  if (fd < 0) {
    return fd;
  }

  fdset->fds[fd].file = file;
  fd_reference(&fdset->fds[fd]);
  return fd;
}

/** Clone or copy the fdset.
 */
int fdset_clone(fdset_t *fdset, int clone)
{
  pthread_mutex_lock(&(*fdset)->mutex);
  fdset_t set = *fdset;
  ASSERT(set->refcnt > 0);

  if (clone) {
    // The parent and child are sharing the set.
    ++set->refcnt;
    pthread_mutex_unlock(&set->mutex);
    return 0;
  }

  fdset_t new;
  int s = fdset_new(&new);
  if (s != 0) {
    pthread_mutex_unlock(&set->mutex);
    return s;
  }

  s = addfd(new, set->count - 1, NULL);
  if (s != 0) {
    fdset_release(new);
    pthread_mutex_unlock(&set->mutex);
    return s;
  }

  ASSERT(new->count == set->count);
  size_t size = new->count * sizeof(fd_t);
  memcpy(new->fds, set->fds, size);
  pthread_mutex_unlock(&set->mutex);
  fdset_reference(new);         // Add a reference to all the open files.
  *fdset = new;
  return 0;
}

/** Add a specific file descriptor to a set.
 */
int fdset_addfd(fdset_t fdset, int spec, file_t file)
{
  pthread_mutex_lock(&fdset->mutex);
  int fd = addfd(fdset, spec, file);
  pthread_mutex_unlock(&fdset->mutex);
  return fd;
}

/** Create a file descriptor and add it to a set.
 */
int fdset_add(fdset_t fdset, file_t file)
{
  return fdset_addfd(fdset, -1, file);
}

/** Remove a file descriptor from a set.
 */
int fdset_remove(fdset_t fdset, int fd)
{
  pthread_mutex_lock(&fdset->mutex);
  if (fd >= fdset->count || fdset->fds[fd].file == NULL) {
    pthread_mutex_unlock(&fdset->mutex);
    return -EBADF;
  }

  fd_release(&fdset->fds[fd]);
  pthread_mutex_unlock(&fdset->mutex);
  return 0;
}

/** Get a file pointer corresponding to a file descriptor.
 */
int fdset_getfile(fdset_t fdset, int fd, file_t *filep)
{
  pthread_mutex_lock(&fdset->mutex);
  if (fd >= fdset->count || fdset->fds[fd].file == NULL) {
    // This is not an active file descriptor.
    pthread_mutex_unlock(&fdset->mutex);
    return -EBADF;
  }

  *filep = fdset->fds[fd].file;
  pthread_mutex_unlock(&fdset->mutex);
  return fd;
}

/** Dup a file descriptor in a set.
 */
int fdset_dup(fdset_t fdset, int fd)
{
  pthread_mutex_lock(&fdset->mutex);
  if (fd >= fdset->count || fdset->fds[fd].file == NULL) {
    pthread_mutex_unlock(&fdset->mutex);
    return -EBADF;
  }

  int newfd = addfd(fdset, -1, fdset->fds[fd].file);
  pthread_mutex_unlock(&fdset->mutex);
  return newfd;
}

/** Dup a specific file descriptor.
 * If free, find the first available free file descriptor.
 */
int fdset_getdup(fdset_t fdset, int fd, file_t *filep, int free)
{
  pthread_mutex_lock(&fdset->mutex);
  if (!free) {
    // We need a specific file descriptor (dup2).
    if (fd < fdset->count) {
      *filep = fdset->fds[fd].file;
      pthread_mutex_unlock(&fdset->mutex);
      return fd;
    }
  } else {
    // We need the next open  file descriptor (fcntl(FD_DUPFD)).
    while (fd < fdset->count) {
      if (!free || fdset->fds[fd].file == NULL) {
        // Have an available file descriptor.
        *filep = fdset->fds[fd].file;
        pthread_mutex_unlock(&fdset->mutex);
        return fd;
      }

      ++fd;
    }
  }

  // Try to allocate a new file descriptor.
  int error;
  if ((error = addfd(fdset, fd,  NULL)) < 0) {
    pthread_mutex_unlock(&fdset->mutex);
    return error;
  }

  *filep = NULL;
  pthread_mutex_unlock(&fdset->mutex);
  return fd;
}

/** Set a file pointer corresponding to a file descriptor.
 */
int fdset_setfile(fdset_t fdset, int fd, file_t file)
{
  pthread_mutex_lock(&fdset->mutex);
  if (fd >= fdset->count) {
    pthread_mutex_unlock(&fdset->mutex);
    return -EBADF;
  }

  fdset->fds[fd].file = file;
  pthread_mutex_unlock(&fdset->mutex);
  return fd;
}

/** Create a new fdset.
 */
int fdset_new(fdset_t *fdset)
{
  *fdset = kmem_alloc(sizeof(struct fdset));
  if (*fdset == NULL) {
    return -EMFILE;
  }

  pthread_mutex_init(&(*fdset)->mutex, NULL);
  (*fdset)->refcnt = 1;
  (*fdset)->count = 0;
  (*fdset)->fds = NULL;
  return 0;
}
