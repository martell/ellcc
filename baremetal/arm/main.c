/* A simple bare metal main.
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    // __syscall(1, 2, 3, 4, 5, 6, 7);
    printf("%s: hello world\n", argv[0]);
    printf("hello world\n");
    for ( ;; ) {
        char buffer[100];
        fgets(buffer, sizeof(buffer), stdin);
        printf("got: %s", buffer);
    }
}


