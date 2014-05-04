/** The command processor interface.
 */
#ifndef _command_h_
#define _command_h_

typedef int (*CommandFn)(int, char **argv);     // A command callback function.
enum { COMMAND_OK, COMMAND_ERROR };             // Callback return values.

/** Insert a command into the command table.
 */
void command_insert(const  char *name, CommandFn fn);

/** The kernel command handling loop.
 * This never returns.
 */
void do_commands(const char *prompt);

#endif
