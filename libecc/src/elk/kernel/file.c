/** File handling.
 */
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "kernel.h"
#include "file.h"
#include "vnode.h"

/** Release a file descriptor.
 */
static void fd_release(fd_t **fd)
{
  if (fd == NULL || *fd == NULL) {
    return;
  }

  pthread_mutex_lock(&(*fd)->mutex);
  if (--(*fd)->f_count == 0) {
    file_t fp = (*fd)->file;
    if (fp)
      vfs_close(fp);
    pthread_mutex_unlock(&(*fd)->mutex);
    free(*fd);
    *fd = NULL;
    return;
  }
  pthread_mutex_unlock(&(*fd)->mutex);
}

/** Add a reference to a file descriptor.
 */
static void fd_reference(fd_t *fd)
{
  if (fd == NULL) {
    return;
  }

  pthread_mutex_lock(&fd->mutex);
  ++fd->f_count;
  file_t fp = fd->file;
  if (fp) {
    vref(fp->f_vnode);
    ++fp->f_count;
  }

  pthread_mutex_unlock(&fd->mutex);
}

/** Add a reference to a set of file descriptors.
 */
static void fdset_reference(fdset_t *fdset)
{
  for (int i = 0; i < fdset->count; ++i) {
    fd_reference(fdset->fds[i]);
  }
}

/** Release a set of file descriptors.
 */
void fdset_release(fdset_t *fdset)
{

  for (int i = 0; i < fdset->count; ++i) {
    fd_release(&fdset->fds[i]);
  }

  free(fdset->fds);
  fdset->fds = NULL;
  fdset->count = 0;
}

/** Clone or copy the fdset.
 */
int fdset_clone(fdset_t *fdset, int clone)
{
  if (fdset->count == 0) {
    return 0;
  }

  if (clone) {
    // The parent and child are sharing the set.
    size_t size = fdset->count * sizeof(fd_t *);
    fd_t **newset = malloc(size);
    if (newset == NULL) {
      return -EMFILE;
    }

    memcpy(newset, fdset->fds, size);
    fdset->fds = newset;
    fdset_reference(fdset);
    return 0;
  }

  // Make a copy of the fdset.
  // RICH: fdset_t orig = *fdset;
  return -ENOSYS;
}

/** Get an available entry in an fdset.
 */
static int fdset_grow(fdset_t *fdset, int spec)
{
  int s;
  int initfds = INITFDS;
  if (spec == -1) {
    spec = 0;
  } else if (fdset->fds == NULL) {
    initfds = spec + 1;
  }

  if (fdset->fds == NULL) {
    fdset->fds = malloc(initfds * sizeof(fd_t *));
    if (fdset->fds == NULL) {
      return -EMFILE;
    }

    fdset->count = initfds;
    for (int i = 0; i < initfds; ++i) {
      fdset->fds[i] = NULL;
    }

    s = 0;      // Allocate the first file descriptor.
  } else {
    for (s = spec; s < fdset->count; ++s) {
      if (fdset->fds[s] == NULL) {
        break;
      }
    }

    if (s >= fdset->count) {
      // No open slot found, double the size of the fd array.
      fd_t **newfds = realloc(fdset->fds,
                              (s * FDMULTIPLIER) * sizeof(fd_t *));
      if (newfds == NULL) {
        return -EMFILE;
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
    return -EMFILE;
  }

  pthread_mutex_init(&fd->mutex, NULL);
  fd->f_count = 0;
  fd->file = NULL;
  *res = fd;
  return 0;
}

/** Allocate a new file descriptor.
 */
static int fd_allocate(fdset_t *fdset, int spec)
{
  int fd = fdset_grow(fdset, spec);
  if (fd < 0) {
    return fd;
  }

  int s = newfd(&fdset->fds[fd]);
  if (s < 0) {
    return s;
  }

  return fd;
}

/** Add a file specific file descriptor to a set.
 */
int fdset_addfd(fdset_t *fdset, int spec, file_t file)
{
  int fd = fd_allocate(fdset, spec);
  if (fd < 0) {
    return fd;
  }

  fdset->fds[fd]->file = file;
  fd_reference(fdset->fds[fd]);

  return fd;
}

/** Create a file descriptor and add it to a set.
 */
int fdset_add(fdset_t *fdset, file_t file)
{
  return fdset_addfd(fdset, -1, file);
}

/** Dup a file descriptor in a set.
 */
int fdset_dup(fdset_t *fdset, int fd)
{
  if (fd >= fdset->count || fdset->fds[fd] == NULL) {
    return -EBADF;
  }

  return fdset_add(fdset, fdset->fds[fd]->file);
}

/** Remove a file descriptor from a set.
 */
int fdset_remove(fdset_t *fdset, int fd)
{
  if (fd >= fdset->count || fdset->fds[fd] == NULL) {
    return -EBADF;
  }

  fd_release(&fdset->fds[fd]);
  return 0;
}

