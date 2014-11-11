/** Kernel command processing.
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <kernel.h>
#include <command.h>

// Make the command processor a loadable feature.
FEATURE(command, command)

#define MAXLINE 1024        // The maximum command line length.
#define MAXCOMMANDS 128     // The maximum number of commands supported.

typedef struct command {
  const char *name;                   	// The command name.
  CommandFn fn;                         // The callback.
} Command;

// The command lock.
static lock_t lock;
static int commands = 1;                // The number of commands defined.

// The  command table.
static struct command command_table[MAXCOMMANDS];

/** Insert a command into the command table.
 */
void command_insert(const  char *name, CommandFn fn)
{
  if (commands >= MAXCOMMANDS) {
    return;
  }

  __elk_lock_aquire(&lock);
  command_table[commands] = (Command){ name, fn };
  ++commands;
  __elk_lock_release(&lock);
}

/** Parse a string for "words".
 * If argv is != NULL, save the pointer to the first character in argv[count]
 * otherwise just count words.
 * If no substitution is done, point to the original word.
 * If substitution is done, point to a newly allocated string.
 */
static int parse_words(char *string, char **argv)
{
  int count = 0;              // The word counter.
  int delim = 0;              // The string deliminator.

  // Skip leading whitespace.
  do {
    while (isspace(*string)) {
      ++string;
    }

    // Find the next word.
    delim = 0;                      	// Default to whitespace.
    switch (*string) {
    case '\'':
      delim = '\'';
      ++string;
      break;
    case '"':
      delim = '"';
      ++string;
      break;
    }

    if (*string) {
      if (argv) {
	argv[count] = string;   	// The beginning of the next word.
      }
      ++count;                    	// Count the words.
    }

    // Find the end of the word.
    while (*string) {
      if (delim) {
	if (*string == delim) {
	  delim = 0;
	  break;
	}
      } else if (isspace(*string)) {
	  break;
      }

      // Ignore everything else.
      ++string;
    }

    if (*string == '\0')
      break;

    // Terminate the word.
    if (argv)
      *string = '\0';

    ++string;
  } while (*string);

  if (delim) {
    // A malformed string.
    fprintf(stderr, "argument %d is missing a trailing quote\n", count);
    return -1;
  }

  return count;
}

static void free_args(char **argv)
{
  if (argv == NULL)
    return;

  // Adjust to the beginning of the argv array.
  argv -= 2;
  // Free any malloc()'d words.
  for (int i = 2; argv[i]; i++) {
    if (argv[i] < argv[0] || argv[i] >= argv[1]) {
      free(argv[i]);
    }
  }

  // Free the copy of the string.
  free(argv[0]);
  // Free  the string vector.
  free(argv);
}

static int parse_args(const char *string, char ***av)
{
  int count = parse_words((char *)string, NULL);
  if (count <= 0) {
    *av = NULL;
    return count;
  }

  // Allocate enough room for three more pointers:
  // the start and end of the string and a trailing NULL.
  char **argv = malloc((count + 3) * sizeof(char *));
  if (argv == NULL) {
    fprintf(stderr, "out of memory\n");
    return -1;
  }

  size_t length = strlen(string) + 1;
  argv[0] = malloc(length);
  memcpy(argv[0], string, length);
  if (argv[0] == NULL) {
    free(argv);
    fprintf(stderr, "out of memory\n");
    return -1;
  }

  // Remember the end of the string.
  argv[1] = argv[0] + length;

  // Create the argv vector.
  int argc = parse_words(argv[0], argv  + 2);

  if (argc != count) {
    // Internal error.
    free_args(argv + 2);
    return -1;
  }

  // Terminate the list.
  argv[count + 2] = NULL;
  *av = argv + 2;
  return count;
}

/** Find and run a command.
 */
int run_command(int argc, char **argv)
{
  for (int i = 0; i < commands; ++i) {
    if (strcmp(argv[0], command_table[i].name) == 0) {
      int s = command_table[i].fn(argc, argv);
      return s;
    }
  }

  printf("unrecognized command: %s\n", argv[0]);
  return COMMAND_ERROR;
}

static int do_command(const char *string)
{
  char **argv;
  int argc = parse_args(string, &argv);
  if (argc <= 0) {
    return COMMAND_OK;
  }
  int s = run_command(argc, argv);
  if (s == COMMAND_OK) {
    free_args(argv);
    return s;
  }

  free_args(argv);
  return COMMAND_ERROR;
}

void do_commands(const char *prompt)
{
  // Enter the kernel command loop.
  for ( ;; ) {
    char buffer[MAXLINE];
    printf("%s %% ", prompt);
    fflush(stdout);
    fgets(buffer, sizeof(buffer), stdin);
    do_command(buffer);
  }
}

/* The help coommand.
 * Call each command function in turn with argc == 0,
 * or a set of command functions each with argc == -1.
 */
static int helpCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("print information about commands.\n");
    printf("%sIf arguments are given, each command in turn\n", *argv);
    printf("%swill display (sometimes) longer information.\n", *argv);
    return COMMAND_OK;
  }

  // 22 spaces (20 + ": ").
  static const char *spaces = "                      ";

  if (argc == 1) {
    // The simple case.
    for (int i = 1; i < commands; ++i) {
      if (command_table[i].name) {
	printf("%20.20s: ", command_table[i].name);
      }

      command_table[i].fn(0, (char **)&spaces);
    }
  } else {
    for (int count = 1; count < argc; ++count) {
      for (int i = 0; i < commands; ++i) {
	if (command_table[i].name &&
	  strcmp(argv[count], command_table[i].name) == 0) {
	  printf("%20.20s: ", command_table[i].name);
	  command_table[i].fn(-1, (char **)&spaces);
	}
      }
    }
  }
  return COMMAND_OK;
}

static int repeatCommand(int argc, char **argv)
{
  if (argc <= 0) {
    printf("repeat the specified command with arguments.\n");
    return COMMAND_OK;
  }

  if (argc < 3) {
    printf("usage: %s <count> <command>\n", argv[0]);
    return COMMAND_ERROR;
  }

  int count = strtol(argv[1], NULL, 0);
  while (count--) {
    if (run_command(argc - 2, argv + 2) != COMMAND_OK) {
      return COMMAND_ERROR;
    }
  }
  return COMMAND_OK;
}

/* Initialize the command processor.
 */
ELK_CONSTRUCTOR()
{
    // The help command is the first in the table.
    command_table[0] = (Command){ "help", helpCommand };
    command_insert("repeat", repeatCommand);
}
