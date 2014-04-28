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

enum redirection_type
  {
    SIMPLE_IN,		// A < B
    SIMPLE_OUT,		// A > B
    CONCAT_OUT,		// A >> B
    IN_AMP,		// A<&B
    OUT_AMP,		// A>&B
    IN_OUT,		// A<>B
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
  /* append field determines whether to
	0: overwrite
	1: append
	2: prepend 		     */
  int append;
  /* I/O file descriptors
	[fd_n]<&word_ifd
	[fd_n]>&word_ofd
	[fd_n]<&digit_ifd
	[fd_n]>&digit_ofd
				     */
  int fd_n;
  char* word_ifd;
  char* word_ofd;
  int digit_ifd;
  int digit_ofd;
  /* pipe = true
     >| = false; 		     */
  enum redirection_type *redir_array;
  int numOfRedir;
  bool pipe;

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
    int pid;
    struct command *cmd;
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
