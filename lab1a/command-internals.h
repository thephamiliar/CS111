// UCLA CS 111 Lab 1 command internals

#include <stdlib.h> //for pid in dep_node
#include <unistd.h>

enum command_type
  {
    AND_COMMAND,         // A && B
    SEQUENCE_COMMAND,    // A ; B
    OR_COMMAND,          // A || B
    PIPE_COMMAND,        // A | B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
    SUBSHELL_COMMAND2,	 // )
  };

// Data associated with a command.
struct command
{
  enum command_type type;

  // Exit status, or -1 if not known (e.g., because it has not exited yet).
  int status;

  // I/O redirections, or null if none.
  char *input;
  char *output;

  union
  {
    // for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
    struct command *command[2];

    // for SIMPLE_COMMAND:
    char **word;

    // for SUBSHELL_COMMAND:
    struct command *subshell_command;
  } u;
};

struct command_stream
{
    struct cmd_node *root;
    struct cmd_node *last;
};

struct cmd_node
{
    struct command *c;
    struct cmd_node *next;
};

// Node for dependency tree
struct dep_node
{
    pid_t pid;
    char **in;    
    char **out;
    int numOfInput;  // could be more than one input/output dependency
    int numOfOutput; 
    struct dep_node **dependency;
    int numOfDependencies; 
    bool isDone;    
};

// global array to access dependency nodes
extern struct dep_node **dep_access;
extern int numOfDepNodes;
