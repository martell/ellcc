/** File handling.
 */
#include "kernel.h"
#include "file.h"

typedef struct file
{
  lock_t lock;                  // The lock protecting the file.
  int references;               // The number of references to this file.
} file_t;

typedef struct fd
{
  lock_t lock;                  // The lock protecting the file descriptor.
  int references;               // The number of references to this descriptor.
  file_t *file;                 // The file accessed by the file decriptor.
} fd_t;

// A set of file descriptors.
struct fd_set
{
  lock_t lock;                  // The lock protecting the set.
  int references;               // The number of references to this set.
  int count;                    // Number of file descriptors in the set.
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

/** Release a set of file descriptors.
 */
void __elk_fdset_release(fdset_t *set)
{
  if (*set == NULL) {
    // No descriptors allocated.
    return;
  }

  __elk_lock_aquire(&(*set)->lock);

  if (--(*set)->references == 0) {
    // This set is done being used.
    for (int i = 0; i < (*set)->count; ++i) {
      fd_release((*set)->fds[i]);
    }
    __elk_lock_release(&(*set)->lock);
    free(*set);
    *set = NULL;
    return;
  }

  __elk_lock_release(&(*set)->lock);
}

