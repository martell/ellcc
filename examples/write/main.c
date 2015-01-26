#include <unistd.h>

int main()
{
    write(1, "hello world\n", sizeof("hello world\n"));
}
