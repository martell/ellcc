// XFAIL: *
// RUN: %ppc64ecc -o %t %s && %ppc64run %t
#include "../ecc_test.h"
#include <signal.h>

static volatile sig_atomic_t i = 0;

static void handler(int sig)
{
    i = 1;
}

TEST_GROUP(Signal)
    TEST(raise(SIGINT) == 0, "Raise a signal");
    TEST(i == 1, "The signal handler has been called");
END_GROUP

