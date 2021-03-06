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

#ifndef _SYS_BUF_H_
#define _SYS_BUF_H_

#include <sys/types.h>
#include <pthread.h>
#include "config.h"
#include "list.h"

/*
 * Buffer header
 */
struct buf {
  struct list b_link;                   // Link to block list.
  int b_flags;                          // See defines below.
  dev_t b_dev;                          // Device number.
  int b_blkno;                          // Block # on device.
  pthread_mutex_t b_lock;               // Lock for access.
  char *b_data;                         // Pointer to data buffer.
};

/*
 * These flags are kept in b_flags.
 */
#define B_BUSY   0x00000001             // I/O in progress.
#define B_DELWRI 0x00000002             // Delay I/O until buffer reused.
#define B_INVAL  0x00000004             // Does not contain valid info.
#define B_READ   0x00000008             // Read buffer.
#define B_DONE   0x00000010             // I/O completed.

#if ELK_NAMESPACE
#define getblk __elk_getblk
#define bread __elk_bread
#define bwrite __elk_bwrite
#define bdwrite __elk_bdwrite
#define binval __elk_binval
#define brelse __elk_brelse
#define bflush __elk_bflush
#define bio_sync __elk_bio_sync
#define bio_init __elk_bio_init
#endif

struct buf *getblk(dev_t, int);
int bread(dev_t, int, struct buf **);
int bwrite(struct buf *);
void bdwrite(struct buf *);
void binval(dev_t);
void brelse(struct buf *);
void bflush(struct buf *);
void bio_sync(void);
void bio_init(void);

#endif /* !_SYS_BUF_H_ */
