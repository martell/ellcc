There are a few examples of using ELK in different configurarions.
Here is what's in the directories:

* main          The simplest program: an empty main().
                No I/O, no threads, no files, etc.
* hello         The simplest program with I/O. "hello world".
                Uses printf() but no threads, no files, etc.
* hellocpp      The C++ version of hello.
                Uses cout but no threads, no files, etc.
* threads       A simple configuration enabling multi-threading and
                interrupt driven I/O.
* elk           All ELK features. Threads, file systems, virtual memory ...
