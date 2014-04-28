// UCLA CS 111 Lab 1 command printing, for debugging

#include "command.h"
#include "command-internals.h"

#include <stdio.h>
#include <stdlib.h>

static void
command_indented_print (int indent, command_t c)
{
  switch (c->type)
    {
    case AND_COMMAND:
    case SEQUENCE_COMMAND:
    case OR_COMMAND:
      {
	command_indented_print (indent + 2 * (c->u.command[0]->type != c->type),
				c->u.command[0]);
	static char const command_label[][3] = { "&&", ";", "||" };
	printf (" \\\n%*s%s\n", indent, "", command_label[c->type]);
	command_indented_print (indent + 2 * (c->u.command[1]->type != c->type),
				c->u.command[1]);
	break;
      }
    case PIPE_COMMAND:
      {
	if (c->pipe == true)
	{
		command_indented_print (indent + 2 * (c->u.command[0]->type != c->type),
				c->u.command[0]);
		static char const command_label[][2] = { "|" };
		printf (" \\\n%*s%s\n", indent, "", command_label[c->type]);
		command_indented_print (indent + 2 * (c->u.command[1]->type != c->type),
				c->u.command[1]);
	}
	else
	{
		command_indented_print (indent + 2 * (c->u.command[0]->type != c->type),
				c->u.command[0]);
		static char const command_label[][3] = { ">|" };
		printf (" \\\n%*s%s\n", indent, "", command_label[c->type]);
		command_indented_print (indent + 2 * (c->u.command[1]->type != c->type),
				c->u.command[1]);
	}
	break;
      }
    case SIMPLE_COMMAND:
      {
	char **w = c->u.word;
	printf ("%*s%s", indent, "", *w);
	while (*++w)
	  printf (" %s", *w);
	break;
      }

    case SUBSHELL_COMMAND:
      printf ("%*s(\n", indent, "");
      command_indented_print (indent + 1, c->u.subshell_command);
      printf ("\n%*s)", indent, "");
      break;

    default:
      abort ();
    }
  
  
  if (c->input)
    printf ("<%s", c->input);
  if (c->output)
  {
    // added >> feature and options
    if (c->append == 0)
   	printf (">%s", c->output);
    else if (c->append == 2)
	printf (" -b >>%s", c->output);
    else //c->append == 1
	printf (">>%s", c->output);
    // end of >> feature and options
  }
  /*if (c->fd_n > -1)
  {
  	if (c->word_ifd)
  	{
		printf ("%d<&%s", c->fd_n, c->word_ifd);
  	}
  	else if (c->word_ofd)
 	 {
   		printf ("%d>&%s", c->fd_n, c->word_ofd);
 	 }
 	 else if (c->digit_ifd > -1)
 	 {
   		printf ("%d<&%d", c->fd_n, c->digit_ifd);
 	 }
 	 else if (c->digit_ofd > -1)
 	 {
   		printf ("%d>&%d", c->fd_n, c->digit_ofd);
	}
   }*/
}

void
print_command (command_t c)
{
  command_indented_print (2, c);
  putchar ('\n');
}
