/*	$NetBSD: sem.c,v 1.5 2008/11/14 15:49:20 ad Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (C) 2000 Jason Evans <jasone@freebsd.org>.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice(s), this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified other than the possible
 *    addition of one or more copyright notices.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice(s), this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER(S) ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: sem.c,v 1.5 2008/11/14 15:49:20 ad Exp $");

/*
 * If an application is linked against both librt and libpthread, the
 * libpthread versions must be used.  Provide weak aliases to cause
 * this behavior.
 */

#define	_LIBC

#include <sys/types.h>
// RICH: #include <sys/ksem.h>
#include <sys/queue.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>

struct _sem_st {
	unsigned int	ksem_magic;
#define	KSEM_MAGIC	0x90af0421U

	LIST_ENTRY(_sem_st) ksem_list;
	intptr_t		ksem_semid;	/* 0 -> user (non-shared) */
	sem_t		*ksem_identity;
};

static int sem_alloc(unsigned int value, intptr_t semid, sem_t *semp);
static void sem_free(sem_t sem);

static LIST_HEAD(, _sem_st) named_sems = LIST_HEAD_INITIALIZER(&named_sems);

int sem_init(sem_t *sem, int pshared, unsigned int value)
    __weak_alias(_librt_sem_init);
int sem_destroy(sem_t *sem) __weak_alias(_librt_sem_destroy);
sem_t *sem_open(const char *name, int oflag, ...)
    __weak_alias(_librt_sem_open);
int sem_close(sem_t *sem) __weak_alias(_librt_sem_close);
int sem_unlink(const char *name) __weak_alias(_librt_sem_unlink);
int sem_wait(sem_t *sem) __weak_alias(_librt_sem_wait);
int sem_trywait(sem_t *sem) __weak_alias(_librt_sem_trywait);
int sem_post(sem_t *sem) __weak_alias(_librt_sem_post);
int sem_getvalue(sem_t * __restrict sem, int * __restrict sval)
    __weak_alias(_librt_sem_getvalue);

static void
sem_free(sem_t sem)
{

	sem->ksem_magic = 0;
	free(sem);
}

static int
sem_alloc(unsigned int value, intptr_t semid, sem_t *semp)
{
	sem_t sem;

	if (value > SEM_VALUE_MAX)
		return (EINVAL);

	if ((sem = malloc(sizeof(struct _sem_st))) == NULL)
		return (ENOSPC);

	sem->ksem_magic = KSEM_MAGIC;
	sem->ksem_semid = semid;
 
	*semp = sem;
	return (0);
}

/* ARGSUSED */
int
_librt_sem_init(sem_t *sem, int pshared, unsigned int value)
{
	intptr_t	semid;
	int error;

#if RICH
	if (_ksem_init(value, &semid) == -1)
		return (-1);
#endif

	if ((error = sem_alloc(value, semid, sem)) != 0) {
		// RICH: _ksem_destroy(semid);
		errno = error;
		return (-1);
	}

	return (0);
}

int
_librt_sem_destroy(sem_t *sem)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->ksem_magic != KSEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif

#if RICH
	if (_ksem_destroy((*sem)->ksem_semid) == -1)
		return (-1);
#endif

	sem_free(*sem);

	return (0);
}

sem_t *
_librt_sem_open(const char *name, int oflag, ...)
{
	sem_t *sem, s;
	intptr_t semid;
	mode_t mode;
	unsigned int value;
	int error;
	va_list ap;

	mode = 0;
	value = 0;

	if (oflag & O_CREAT) {
		va_start(ap, oflag);
#if RICH // This causes a ppc code generation error: Track it down.
		mode = va_arg(ap, int);
		value = va_arg(ap, unsigned int);
#endif
		va_end(ap);
	}

	/*
	 * We can be lazy and let the kernel handle the oflag,
	 * we'll just merge duplicate IDs into our list.
	 */
#if RICH
	if (_ksem_open(name, oflag, mode, value, &semid) == -1)
		return (SEM_FAILED);
#endif

	/*
	 * Search for a duplicate ID, we must return the same sem_t *
	 * if we locate one.
	 */
	LIST_FOREACH(s, &named_sems, ksem_list) {
		if (s->ksem_semid == semid)
			return (s->ksem_identity);
	}

	if ((sem = malloc(sizeof(*sem))) == NULL) {
		error = ENOSPC;
		goto bad;
	}
	if ((error = sem_alloc(value, semid, sem)) != 0)
		goto bad;

	LIST_INSERT_HEAD(&named_sems, *sem, ksem_list);
	(*sem)->ksem_identity = sem;

	return (sem);

 bad:
	// RICH: _ksem_close(semid);
	if (sem != NULL) {
		if (*sem != NULL)
			sem_free(*sem);
		free(sem);
	}
	errno = error;
	return (SEM_FAILED);
}

int
_librt_sem_close(sem_t *sem)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->ksem_magic != KSEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif

#if RICH
	if (_ksem_close((*sem)->ksem_semid) == -1)
		return (-1);
#endif

	LIST_REMOVE((*sem), ksem_list);
	sem_free(*sem);
	free(sem);
	return (0);
}

int
_librt_sem_unlink(const char *name)
{

	// RICH: return (_ksem_unlink(name));
        return -1; // RICH
}

int
_librt_sem_wait(sem_t *sem)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->ksem_magic != KSEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif

	// RICH: return (_ksem_wait((*sem)->ksem_semid));
        return -1; // RICH
}

int
_librt_sem_trywait(sem_t *sem)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->ksem_magic != KSEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif

	// RICH: return (_ksem_trywait((*sem)->ksem_semid));
        return -1; // RICH
}

int
_librt_sem_post(sem_t *sem)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->ksem_magic != KSEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif

	// RICH: return (_ksem_post((*sem)->ksem_semid));
        return -1; // RICH
}

int
_librt_sem_getvalue(sem_t * __restrict sem, int * __restrict sval)
{

#ifdef ERRORCHECK
	if (sem == NULL || *sem == NULL || (*sem)->ksem_magic != KSEM_MAGIC) {
		errno = EINVAL;
		return (-1);
	}
#endif
	// RICH: return (_ksem_getvalue((*sem)->ksem_semid, sval));
        return -1; // RICH
}