/* A BSD compatability file that creates sys_siglist[] on the fly
 * using strsignal().
 */

#include <string.h>
#include <signal.h>
#include <libc.h>

const char *_sys_siglist[_NSIG] = {};

static void getsiglist(void)
    __attribute__((__constructor__, __used__));

static void getsiglist(void)
{
    for (int i = 0; i < _NSIG; ++i) {
        _sys_siglist[i] = strsignal(i);
    }
}

weak_alias(_sys_siglist, sys_siglist);
