This is a simple "hello world" example in C++. This example can be built for
several Linux and standalone targets. Type "make help" to see the
available target configurations.

This example uses simple polled I/O for printing characters under ELK.

To build this example, typical commands would be:
    make arm-elk-engeabi
    make run
    make debug
    (and then in another window)
    make tui  # "make gdb" starts gdb in commmand mode.

Currently supported configuration(s):
Linux:
  armeb-linux-engeabi armeb-linux-engeabihf arm-linux-engeabi
  arm-linux-engeabihf i386-linux-eng mipsel-linux-eng
  mipsel-linux-engsf mips-linux-eng mips-linux-engsf ppc-linux-eng
  x86_64-linux-eng
ELK:
  arm-elk-engeabi
