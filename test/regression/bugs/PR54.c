// XFAIL: *
// FIXES: test in stdlib/inttypes.c.
// RUN: %microblazeecc -o %t %s -lm && %microblazerun %t
// RUN: %microblazeelecc -o %t %s -lm && %microblazeelrun %t
#include <stdio.h>

int main()
{
    unsigned long long x, y;            

    x = 100;      
    y = 0x8000000000000000ULL;
    return !(x > y);
}
