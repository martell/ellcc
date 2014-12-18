This is a simple empty main() example. This example can be built for
several Linux and standalone targets. Type "make help" to see the
available target configurations.

This is the simplest configuration of ELK possible. It only executes
the startup code and initializes the C library.

The goal of this example is to be able to debug, set a breakpoint at main()
and verify that the startup code gets us there.

To build this example, typical commands would be:
    make arm-elk-engeabi
    make run
    make debug
    (and then in another window)
    make tui  # "make gdb" starts gdb in commmand mode.

About the only thing you can do with this example is to single step through
the startup code or set a breakpoint at a function (e.g. main()) and hit
"c" to continue to the breakpoint.

Currently supported configuration(s):
Linux:
  armeb-linux-engeabi armeb-linux-engeabihf arm-linux-engeabi
  arm-linux-engeabihf i386-linux-eng microblaze-linux-eng mipsel-linux-eng
  mipsel-linux-engsf mips-linux-eng mips-linux-engsf ppc-linux-eng
  x86_64-linux-eng
ELK:
  arm-elk-engeabi
