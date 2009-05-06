#ifndef __PROCESS_H_
#define __PROCESS_H_

#include <_ansi.h>
_BEGIN_STD_C

int execl(const char *path, const char *argv1, ...);
int execle(const char *path, const char *argv1, ... /*, char * const *envp */);
int execlp(const char *path, const char *argv1, ...);

int execv(const char *path, char * const *argv);
int execve(const char *path, char * const *argv, char * const *envp);
int execvp(const char *path, char * const *argv);

int spawnl(int mode, const char *path, const char *argv0, ...);
int spawnle(int mode, const char *path, const char *argv0, ... /*, char * const *envp */);
int spawnlp(int mode, const char *path, const char *argv0, ...);
int spawnlpe(int mode, const char *path, const char *argv0, ... /*, char * const *envp */);

int spawnv(int mode, const char *path, const char * const *argv);
int spawnve(int mode, const char *path, const char * const *argv, const char * const *envp);
int spawnvp(int mode, const char *path, const char * const *argv);
int spawnvpe(int mode, const char *path, const char * const *argv, const char * const *envp);

int cwait(int *, int, int);

#define _P_WAIT		1
#define _P_NOWAIT	2	/* always generates error */
#define _P_OVERLAY	3
#define _P_NOWAITO	4
#define _P_DETACH	5

#define WAIT_CHILD 1

_END_STD_C

#endif
