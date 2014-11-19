/** The command processor interface.
 */
#ifndef _command_h_
#define _command_h_

typedef int (*CommandFn)(int, char **argv);     // A command callback function.
enum { COMMAND_OK, COMMAND_ERROR };             // Callback return values.

/** Insert a command into the command table.
 */
void command_insert(const  char *name, CommandFn fn);

/** Insert an external command into the command.
 */
void command_insert_external(const  char *name, CommandFn fn);

/** Find and run a command.
 */
int run_command(int argc, char **argv);

/** The kernel command handling loop.
 * This never returns.
 */
void do_commands(const char *prompt);

#endif
