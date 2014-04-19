// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
//additional #include
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

int
command_status (command_t c)
{
  return c->status;
}

void executingSimple(command_t c);
void executingSubshell(command_t c);
void executingAnd(command_t c);
void executingOr(command_t c);
void executingSequence(command_t c);
void executingPipe(command_t c);

void execute_switch(command_t c)
{
	switch(c->type)
	{
	case SIMPLE_COMMAND:
		executingSimple(c);
		break;
	case SUBSHELL_COMMAND:
		executingSubshell(c);
		break;
	case AND_COMMAND:
		executingAnd(c);
		break;
	case OR_COMMAND:
		executingOr(c);
		break;
	case SEQUENCE_COMMAND:
		executingSequence(c);
		break;
	case PIPE_COMMAND:
		executingPipe(c);
		break;
	default:
		error(1, 0, "Not a valid command\n");
		c->status = 1;
	}
}

void executingPipe(command_t c)
{
	pid_t returnedPid;
	pid_t firstPid;
	pid_t secondPid;
	int buffer[2];
	int eStatus;

	if ( pipe(buffer) < 0 )
	{
		error (1, errno, "pipe was not created\n");
	}

	firstPid = fork();
	if (firstPid < 0)
   	{
		error(1, errno, "fork was unsuccessful\n");
	}
	else if (firstPid == 0) //child executes command on the right of the pipe
	{
		close(buffer[1]); //close unused write end

        //redirect standard input to the read end of the pipe
        //so that input of the command (on the right of the pipe)
        //comes from the pipe
		if ( dup2(buffer[0], 0) < 0 )
		{
			error(1, errno, "error with dup2\n");
		}
		execute_switch(c->u.command[1]);
		_exit(c->u.command[1]->status);
	}
	else 
	{
		// Parent process
		secondPid = fork(); //fork another child process
                            //have that child process executes command on the left of the pipe
		if (secondPid < 0)
		{
			error(1, 0, "fork was unsuccessful\n");
		}
        	else if (secondPid == 0)
		{
			close(buffer[0]); //close unused read end
			if(dup2(buffer[1], 1) < 0) //redirect standard output to write end of the pipe
            		{
				error (1, errno, "error with dup2\n");
	    		}
			execute_switch(c->u.command[0]);
			_exit(c->u.command[0]->status);
		}
		else
		{
			// Finishing processes
			returnedPid = waitpid(-1, &eStatus, 0); //this is equivalent to wait(&eStatus);
                        //we now have 2 children. This waitpid will suspend the execution of
                        //the calling process until one of its children terminates
                        //(the other may not terminate yet)

			//Close pipe
			close(buffer[0]);
			close(buffer[1]);

			if (secondPid == returnedPid )
			{
			    //wait for the remaining child process to terminate
				waitpid(firstPid, &eStatus, 0); 
				c->status = WEXITSTATUS(eStatus);
				return;
			}
			
			if (firstPid == returnedPid)
			{
			    //wait for the remaining child process to terminate
   				waitpid(secondPid, &eStatus, 0);
				c->status = WEXITSTATUS(eStatus);
				return;
			}
		}
	}	
}
// redirects inputs and outputs
void setupInOut(command_t c)
{
    int in, out;
    if (c->input != NULL) // check for input redirection  
    {
	in = open(c->input, O_RDONLY);
        if (in < 0)
        {
            error(1, errno, "error with opening file\n");
        }
        if(dup2(in, 0) < 0) // replace stdin with input file
        {
            error(1, errno, "error with input redirection\n");
        }
        if(close(in) < 0)
        {
            error(1, errno, "error with closing file\n");
        }
    }
    if (c->output != NULL) // check for output redirection
    { 
	out = open(c->output, O_WRONLY| O_CREAT| O_TRUNC, 0600);
        if (out < 0)
        {
            error(1, errno, "error with opening file\n");
        }
        if(dup2(out, 1) < 0) // replace stdout with output file
        {
            error(1, errno, "error with output redirection\n");
        }
        if (close(out) < 0)
        {
            error(1, errno, "error with closing file\n");
        }
    }
}

void executingSimple(command_t c)
{
	int eStatus;
	pid_t pid = fork();
        if (pid < 0)
        {       
               error(1, 0, "fork was unsuccessful\n");
        }
	else if (pid == 0) // child process
	{	
		setupInOut(c); // redirects inputs and outputs
		if (strcmp(c->u.word[0],"exec") == 0)
			execvp(c->u.word[1], c->u.word++);
		execvp(c->u.word[0], c->u.word); // execute commands
		error(1, errno, "error with command execution\n"); // execvp shouldn't return unless error
	}
	else //parent process
	{
		waitpid(pid, &eStatus, 0);
		c->status = WEXITSTATUS(eStatus);
	}
}

void executingSubshell(command_t c)
{
	int eStatus;
        pid_t pid = fork();
        if (pid < 0)
        {
               error(1, 0, "fork was unsuccessful\n");
        }
        else if (pid == 0) // child process
        {
                setupInOut(c); // redirects inputs and outputs
                execute_switch(c->u.subshell_command);
        	_exit(command_status(c->u.subshell_command));
	}
        else //parent process
        {
                waitpid(pid, &eStatus, 0);
                c->status = WEXITSTATUS(eStatus);
        }
}

void executingAnd(command_t c)
{
    execute_switch(c->u.command[0]);
    if (command_status(c->u.command[0]) > 0)
    {
	c->status = command_status(c->u.command[0]);
    } else {
	execute_switch(c->u.command[1]);
	c->status = command_status(c->u.command[1]);
    }
}

void executingOr(command_t c)
{
    execute_switch(c->u.command[0]);
    if (command_status(c->u.command[0]) == 0)
    {
	c->status = command_status(c->u.command[0]);
    } else {
	execute_switch(c->u.command[1]);
	c->status = command_status(c->u.command[1]);
    }
}

void executingSequence(command_t c)
{ 
    execute_switch(c->u.command[0]);
    execute_switch(c->u.command[1]);
    c->status = command_status(c->u.command[1]);
}

void
execute_command (command_t c, bool time_travel)
{
   /*FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  
    */
	if (time_travel == false)
	{
		execute_switch(c);
	}
}
