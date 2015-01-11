/*-
 * Copyright (c) 2005-2007, Kohsuke Ohtani
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

#ifndef _DEVICE_H
#define _DEVICE_H

#include <sys/types.h>
#include <stdio.h>
#include "config.h"
#include "kernel.h"
#include "uio.h"

#if ELK_NAMESPACE
#define device_create __elk_device_create
#define device_destroy __elk_device_destroy
#define device_lookup __elk_device_lookup
#define device_valid __elk_device_valid
#define device_reference __elk_device_reference
#define device_release __elk_device_release
#define device_private __elk_device_private
#define device_control __elk_device_control
#define device_broadcast __elk_device_broadcast
#define device_info __elk_device_info
#define device_open __elk_device_open
#define device_close __elk_device_close
#define device_read __elk_device_read
#define device_write __elk_device_write
#define device_poll __elk_device_poll
#define device_ioctl __elk_device_ioctl
#define device_info __elk_device_info
#define enodev __elk_enodev
#define nullop __elk_nullop
#define driver_register __elk_driver_register
#endif

typedef struct device *device_t;
#define NODEV ((device_t)0)

/*
 * Device flags
 *
 * If D_PROT is set, the device can not be opened via devfs.
 */
#define D_CHR  0x00000001       // Character device.
#define D_BLK  0x00000002       // Block device.
#define D_REM  0x00000004       // Removable device.
#define D_PROT 0x00000008       // Protected device.
#define D_TTY  0x00000010       // Tty device.

#if defined(DEFINE_DEVICE_STRINGS)
static struct {
  int flag;
  const char *name;
} device_flag_names[] = {
  { D_CHR, "CHR" },
  { D_BLK, "BLK" },
  { D_REM, "REM" },
  { D_PROT, "PROT" },
  { D_TTY, "TTY" },
  { 0, NULL },
};
#endif

/*
 * Device operations
 */
struct devops {
  int (*open)(device_t, int);
  int (*close)(device_t);
  int (*read)(device_t, struct uio *, size_t *, int);
  int (*write)(device_t, struct uio *, size_t *, int);
  int (*poll)(device_t, int);
  int (*ioctl)(device_t, u_long, void *);
  int (*devctl)(device_t, u_long, void *);
};

typedef int (*devop_open_t)(device_t, int);
typedef int (*devop_close_t)(device_t);
typedef int (*devop_read_t)(device_t, struct uio *, size_t *, int);
typedef int (*devop_write_t)(device_t, struct uio *, size_t *, int);
typedef int (*devop_poll_t)(device_t, int);
typedef int (*devop_ioctl_t)(device_t, u_long, void *);
typedef int (*devop_devctl_t)(device_t, u_long, void *);

int enodev(void);
int nullop(void);

#define  no_open   ((devop_open_t)nullop)
#define  no_close  ((devop_close_t)nullop)
#define  no_read   ((devop_read_t)enodev)
#define  no_write  ((devop_write_t)enodev)
#define  no_poll   ((devop_poll_t)enodev)
#define  no_ioctl  ((devop_ioctl_t)enodev)
#define  no_devctl ((devop_devctl_t)nullop)

/*
 * Driver object
 */
struct driver
{
  const char *name;             // Name of device driver.
  const struct devops *devops;  // device operations.
  size_t devsz;                 // size of private data.
  int flags;                    // state of driver.
  int (*probe)(struct driver *);
  int (*init)(struct driver *);
  int (*unload)(struct driver *);
};

/*
 * Driver flags.
 */
#define DS_INACTIVE 0x00        // Driver is inactive.
#define DS_ALIVE    0x01        // Probe succeded.
#define DS_ACTIVE   0x02        // Intialized.
#define DS_DEBUG    0x04        // Debug.

#if defined(DEFINE_DEVICE_STRINGS)
static struct {
  int flag;
  const char *name;
} driver_flag_names[] = {
  { DS_INACTIVE, "INACTIVE" },
  { DS_ALIVE, "ALIVE" },
  { DS_ACTIVE, "ACTIVE" },
  { DS_DEBUG, "DEBUG" },
  { 0, NULL },
};
#endif

/*
 * Device object
 */
struct device
{
  struct device *next;          // Linkage on list of all devices.
  const struct driver *driver;  // Pointer to the driver object.
  char name[CONFIG_MAXDEVNAME]; // Name of device.
  int flags;                    // D_* flags defined above.
  int active;                   // Device has not been destroyed.
  int refcnt;                   // Reference count.
  void *private;                // Private storage.
};

device_t device_create(const struct driver *, const char *, int);
int device_destroy(device_t);
device_t device_lookup(const char *);
int device_valid(device_t);
int device_reference(device_t);
void device_release(device_t);
void *device_private(device_t);
int device_control(device_t, u_long, void *);
int device_broadcast(u_long, void *, int);

// Device information
struct devinfo {
  u_long cookie;                // Index cookie.
  device_t id;                  // Device id.
  int flags;                    // Device characteristics flags.
  char name[CONFIG_MAXDEVNAME]; // Device name.
};

int device_info(struct devinfo *info);
int device_open(const char *, int, device_t *);

// Device open mode.
#define DO_RDONLY       0x0
#define DO_WRONLY       0x1
#define DO_RDWR         0x2
#define DO_RWMASK       0x3

int device_close(device_t);
int device_read(device_t, struct uio *, size_t *, int);
int device_write(device_t, void *, size_t *, int);
int device_poll(device_t, int);
int device_ioctl(device_t, u_long, void *);
int device_info(struct devinfo *);

/** Register a driver for use.
 */
void driver_register(struct driver *driver);

#endif /* !_DEVICE_H */
