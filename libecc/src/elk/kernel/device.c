/*-
 * Copyright (c) 2005-2009, Kohsuke Ohtani
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * device.c - device I/O support routines
 */

/**
 * The routines in this module have the following role:
 *  - Manage the name space for device objects.
 *  - Forward user I/O requests to the drivers with minimum check.
 */

#include <errno.h>
#include <pthread.h>
#include <string.h>

#define DEFINE_DEVICE_STRINGS
#include "device.h"
#include "thread.h"
#include "kmem.h"
#include "command.h"

/* list head of the devices */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_INIT() do { \
  pthread_mutexattr_t attr; \
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
  pthread_mutex_init(&mutex, &attr); \
  } while (0)

#define LOCK()  pthread_mutex_lock(&mutex)
#define UNLOCK()  pthread_mutex_unlock(&mutex)

static struct device *device_list = NULL;

/*
 * device_create - create new device object.
 *
 * A device object is created by the device driver to provide
 * I/O services to applications.  Returns device ID on
 * success, or 0 on failure.
 */
device_t device_create(const struct driver *drv, const char *name, int flags)
{
  device_t dev;
  size_t len;
  void *private = NULL;

  ASSERT(drv != NULL);

  /* Check the length of name. */
  len = strnlen(name, CONFIG_MAXDEVNAME);
  if (len == 0 || len >= CONFIG_MAXDEVNAME)
    return NULL;

  LOCK();

  /* Check if specified name is already used. */
  if (device_lookup(name) != NULL) {
    printf("duplicate device: %s", name);
    UNLOCK();
    return NULL;
  }

  /*
   * Allocate a device structure and device private data.
   */
  if ((dev = kmem_alloc(sizeof(*dev))) == NULL) {
    printf("device_create");
    UNLOCK();
    return NULL;
  }

  if (drv->devsz != 0) {
    if ((private = kmem_alloc(drv->devsz)) == NULL) {
      kmem_free(dev);
      printf("devsz");
      UNLOCK();
      return NULL;
    }

    memset(private, 0, drv->devsz);
  }

  strlcpy(dev->name, name, len + 1);
  dev->driver = drv;
  dev->flags = flags;
  dev->active = 1;
  dev->refcnt = 1;
  dev->private = private;
  dev->next = device_list;
  device_list = dev;

  UNLOCK();
  return dev;
}

/*
 * Destroy a device object. If some other threads still
 * refer the target device, the destroy operation will be
 * pending until its reference count becomes 0.
 */
int device_destroy(device_t dev)
{

  LOCK();
  if (!device_valid(dev)) {
    UNLOCK();
    return -ENODEV;
  }

  dev->active = 0;
  device_release(dev);
  UNLOCK();
  return 0;
}

/*
 * Look up a device object by device name.
 */
device_t device_lookup(const char *name)
{
  device_t dev;

  for (dev = device_list; dev != NULL; dev = dev->next) {
    if (!strncmp(dev->name, name, CONFIG_MAXDEVNAME))
      return dev;
  }
  return NULL;
}

/*
 * Return device's private data.
 */
void *device_private(device_t dev)
{
  ASSERT(dev != NULL);
  ASSERT(dev->private != NULL);

  return dev->private;
}

/*
 * Return true if specified device is valid.
 */
int device_valid(device_t dev)
{
  device_t tmp;
  int found = 0;

  for (tmp = device_list; tmp != NULL; tmp = tmp->next) {
    if (tmp == dev) {
      found = 1;
      break;
    }
  }
  if (found && dev->active)
    return 1;
  return 0;
}

/*
 * Increment the reference count on an active device.
 */
int device_reference(device_t dev)
{

  LOCK();
  if (!device_valid(dev)) {
    UNLOCK();
    return -ENODEV;
  }

  if (!capable(CAP_SYS_RAWIO)) {
    UNLOCK();
    return -EPERM;
  }

  dev->refcnt++;
  UNLOCK();
  return 0;
}

/*
 * Decrement the reference count on a device. If the reference
 * count becomes zero, we can release the resource for the device.
 */
void device_release(device_t dev)
{
  device_t *tmp;

  LOCK();
  if (--dev->refcnt > 0) {
    UNLOCK();
    return;
  }

  /*
   * No more references - we can remove the device.
   */
  for (tmp = &device_list; *tmp; tmp = &(*tmp)->next) {
    if (*tmp == dev) {
      *tmp = dev->next;
      break;
    }
  }

  kmem_free(dev);
  UNLOCK();
}

/*
 * Open the specified device.
 *
 * Even if the target driver does not have an open
 * routine, this function does not return an error. By
 * using this mechanism, an application can check whether
 * the specific device exists or not. The open mode
 * should be handled by an each device driver if it is
 * needed.
 */
int device_open(const char *name, int mode, device_t *devp)
{
  device_t dev;
  char str[CONFIG_MAXDEVNAME];
  int error;

  error = copyinstr(name, str, CONFIG_MAXDEVNAME);
  if (error)
    return error;

  LOCK();
  if ((dev = device_lookup(str)) == NULL) {
    UNLOCK();
    return -ENXIO;
  }

  error = device_reference(dev);
  if (error) {
    UNLOCK();
    return error;
  }

  UNLOCK();

  const struct devops *ops;
  ops = dev->driver->devops;
  ASSERT(ops->open != NULL);
  error = (*ops->open)(dev, mode);
  if (!error)
    *devp = dev;

  device_release(dev);
  return error;
}

/*
 * Close a device.
 *
 * Even if the target driver does not have close routine,
 * this function does not return any errors.
 */
int device_close(device_t dev)
{
  int error;

  if ((error = device_reference(dev)) != 0)
    return error;

  const struct devops *ops;
  ops = dev->driver->devops;
  ASSERT(ops->close != NULL);
  error = (*ops->close)(dev);

  device_release(dev);
  return error;
}

/*
 * Read from a device.
 *
 * Actual read count is set in "nbyte" as return.
 * Note: The size of one block is device dependent.
 */
int device_read(device_t dev, struct uio *uio, size_t *nbyte, int blkno)
{
  size_t count;
  int error;

  if ((error = device_reference(dev)) != 0)
    return error;

  count = *nbyte;
  const struct devops *ops;
  ops = dev->driver->devops;
  ASSERT(ops->read != NULL);
  error = (*ops->read)(dev, uio, &count, blkno);
  if (!error)
    *nbyte = count;

  device_release(dev);
  return error;
}

/*
 * Write to a device.
 *
 * Actual write count is set in "nbyte" as return.
 */
int device_write(device_t dev, void *buf, size_t *nbyte, int blkno)
{
  size_t count;
  int error;

  if ((error = device_reference(dev)) != 0)
    return error;

  count = *nbyte;
  const struct devops *ops;
  ops = dev->driver->devops;
  ASSERT(ops->write != NULL);
  error = (*ops->write)(dev, buf, &count, blkno);
  if (!error)
    *nbyte = count;

  device_release(dev);
  return error;
}

/*
 * Poll a device.
 */
int device_poll(device_t dev, int flags)
{
  int error;

  if ((error = device_reference(dev)) != 0)
    return error;

  const struct devops *ops;
  ops = dev->driver->devops;
  ASSERT(ops->poll != NULL);
  error = (*ops->poll)(dev, flags);

  device_release(dev);
  return error;
}

/*
 * I/O control request.
 *
 * A command and its argument are completely device dependent.
 * The ioctl routine of each driver must validate the user buffer
 * pointed by the arg value.
 */
int device_ioctl(device_t dev, u_long cmd, void *arg)
{
  int error;

  if ((error = device_reference(dev)) != 0)
    return error;

  const struct devops *ops;
  ops = dev->driver->devops;
  ASSERT(ops->ioctl != NULL);
  error = (*ops->ioctl)(dev, cmd, arg);

  device_release(dev);
  return error;
}

/*
 * Similar to ioctl, but is invoked from
 * other device driver rather than from user application.
 */
int device_control(device_t dev, u_long cmd, void *arg)
{
  int error;

  ASSERT(dev != NULL);

  LOCK();
  const struct devops *ops;
  ops = dev->driver->devops;
  ASSERT(ops->devctl != NULL);
  error = (*ops->devctl)(dev, cmd, arg);
  UNLOCK();
  return error;
}

/*
 * Broadcast devctl command to all device objects.
 *
 * If "force" argument is true, we will continue command
 * notification even if some driver returns an error. In this
 * case, this routine returns EIO error if at least one driver
 * returns an error.
 *
 * If force argument is false, a kernel stops the command processing
 * when at least one driver returns an error. In this case,
 * device_broadcast will return the error code which is returned
 * by the driver.
 */
int device_broadcast(u_long cmd, void *arg, int force)
{
  device_t dev;
  int error, retval = 0;

  LOCK();

  for (dev = device_list; dev != NULL; dev = dev->next) {
    /*
     * Call driver's devctl() routine.
     */
    const struct devops *ops;
    ops = dev->driver->devops;
    if (ops == NULL)
      continue;

    ASSERT(ops->devctl != NULL);
    error = (*ops->devctl)(dev, cmd, arg);
    if (error) {
      DPRINTF(DEVDB_CORE, ("%s returns error=%d for cmd=%ld\n",
         dev->name, error, cmd));
      if (force)
        retval = -EIO;
      else {
        retval = error;
        break;
      }
    }
  }

  UNLOCK();
  return retval;
}

/*
 * Return device information.
 */
int device_info(struct devinfo *info)
{
  u_long target = info->cookie;
  u_long i = 0;
  device_t dev;
  int error = -ESRCH;

  LOCK();
  for (dev = device_list; dev != NULL; dev = dev->next) {
    if (i++ == target) {
      info->cookie = i;
      info->id = dev;
      info->flags = dev->flags;
      strlcpy(info->name, dev->name, CONFIG_MAXDEVNAME);
      error = 0;
      break;
    }
  }

  UNLOCK();
  return error;
}

/** Nonexistent device call.
 */
int enodev(void)
{
  return -ENODEV;
}

/* Generic null operation, always returns success.
 */
int nullop(void)
{
  return 0;
}

/** Register a driver for use.
 */
static pthread_mutex_t dmutex = PTHREAD_MUTEX_INITIALIZER;
static struct driver *driver_table[CONFIG_NDRIVERS];

void driver_register(struct driver *driver)
{
  pthread_mutex_lock(&dmutex);
  for (int i = 0; i < CONFIG_NDRIVERS; ++i) {
    if (driver_table[i]) {
      continue;
    }

    driver_table[i] = driver;
    pthread_mutex_unlock(&dmutex);
    return;
  }

  printf("maximum number of drivers exceeded.\n");
  pthread_mutex_unlock(&dmutex);
}

/** Show device information.
 */
static int dsCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("Show device information.\n");
    return COMMAND_OK;
  }

  printf("%*.*s %7.7s %7.7s %-10.10s \n",
         CONFIG_MAXDEVNAME , CONFIG_MAXDEVNAME , "NAME",
         "ACTIVE", "REFCNT", "FLAGS");
  for (struct device *dp = device_list; dp; dp = dp->next) {
      printf("%*.*s %7s %7d ",
             CONFIG_MAXDEVNAME, CONFIG_MAXDEVNAME, dp->name,
             dp->active ? "yes" : "", dp->refcnt);
      int comma = 0;
      for (int i = 0; device_flag_names[i].name; ++i) {
        if (dp->flags & device_flag_names[i].flag) {
          printf("%s%s", comma ? ", " : "", device_flag_names[i].name);
          comma = 1;
        }
      }

      for (int i = 0; driver_flag_names[i].name; ++i) {
        if (dp->driver->flags & driver_flag_names[i].flag) {
          printf("%s%s", comma ? ", " : "", driver_flag_names[i].name);
          comma = 1;
        }
      }

      printf("\n");
  }

  return COMMAND_OK;
}

/** Show driver information.
 */
static int drvsCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("Show driver information.\n");
    return COMMAND_OK;
  }


  printf("Driver table:\n");
        printf(" probe    init     unload   devops   flags    name\n");
        printf(" -------- -------- -------- -------- -------- -----------\n");

  for (int i = 0; i < CONFIG_NDRIVERS; i++) {
    struct driver *dp = driver_table[i];
    if (dp == NULL) {
      continue;
    }

    printf(" %8p %8p %8p %8p %8lx %s\n",
           dp->probe, dp->init, dp->unload,
           dp->devops, (long)dp->flags, dp->name);
  }
  return COMMAND_OK;
}

/** Create a section heading for the help command.
 */
static int sectionCommand(int argc, char **argv)
{
  if (argc <= 0 ) {
    printf("Device Commands:\n");
  }

  return COMMAND_OK;
}

ELK_PRECONSTRUCTOR()
{
  command_insert(NULL, sectionCommand);
  command_insert("ds", dsCommand);
  command_insert("drvs", drvsCommand);
}

C_CONSTRUCTOR()
{
  LOCK_INIT();

  // Call xxx_probe routine for all drivers.
  struct driver *dp;

  DPRINTF(DEVDB_DRV, ("Probing devices...\n"));

  for (int i = 0; i < CONFIG_NDRIVERS; i++) {
    dp = driver_table[i];
    if (dp == NULL) {
      continue;
    }

    if (dp->probe != NULL) {
      if (dp->probe(dp) == 0)
        dp->flags |= DS_ALIVE;
    } else {
      // No probe method. Mark it alive.
      dp->flags |= DS_ALIVE;
    }
  }

  // Call xxx_init routine for all living drivers.

  for (int i = 0; i < CONFIG_NDRIVERS; i++) {
    dp = driver_table[i];
    if (dp == NULL) {
      continue;
    }

    /* All drivers must have init method */
    ASSERT(dp->init != NULL);

    if (dp->flags & DS_ALIVE) {
      DPRINTF(DEVDB_DRV, ("Initializing %s\n", dp->name));
      if (dp->init(dp) == 0)
        dp->flags |= DS_ACTIVE;
    }
  }
}
