#ifndef _command_h_
#define _command_h_

enum { COMMAND_OK, COMMAND_ERROR };                 // Callback return values.
typedef int (*CommandFn)(int, char **argv);         // A command callback function.

/** Insert a command into the command table.
 */
void command_insert(const  char *name, CommandFn fn);

/** The kernel command handling loop.
 * This never returns.
 */
void do_commands(const char *prompt);

#endif
