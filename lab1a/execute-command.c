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

//global in this file
int dep_node_array_size = 64;
int dep_node_in_array_size = 8;
int dep_node_out_array_size = 8;
dep_node_t *dep_access = NULL;
int numOfDepNodes = 0;
#define DEP_NODE_ARRAY_GROW 64
#define DEP_NODE_IO_ARRAY_GROW 8

// function prototypes for executing commands
void executingSimple(command_t c);
void executingSubshell(command_t c);
void executingAnd(command_t c);
void executingOr(command_t c);
void executingSequence(command_t c);
void executingPipe(command_t c);

// function prototypes for building a dep node
void addingSimpleToDN(command_t c, dep_node_t dn);
void addingSubshellToDN(command_t c,  dep_node_t dn);
void addingSequentialToDN(command_t c, dep_node_t dn);

// helper functions for building a dep node
void init_dep_node(dep_node_t dn);
void grow_in_ptr(dep_node_t dn);
void grow_out_ptr(dep_node_t dn);
void add_in_out(command_t c, dep_node_t dn);
void setup_new_dep_node(void);
void check_for_prev_dependencies(dep_node_t dn);

int
command_status (command_t c)
{
  return c->status;
}

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
		if (strcmp(c->u.word[0],"exec") == 0) // special exception for exec
			c->u.word++;
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

/* TIMETRAVEL FUNCTIONS*/
void make_dependency_list (command_t c, dep_node_t dn)
{
     switch(c->type)
     {
        case SIMPLE_COMMAND:
                addingSimpleToDN(c, dn);
                break;
        case SUBSHELL_COMMAND:
                addingSubshellToDN(c, dn);
                break;
        case AND_COMMAND:
        case OR_COMMAND:
        case SEQUENCE_COMMAND:
        case PIPE_COMMAND:
                addingSequentialToDN(c, dn);
                break;
        default:
                error(1, 0, "Not a valid command\n");     
     }     
    //read command stream
    //while 
	//make depend node depend_t *dpd = (depend_t) checked_malloc(MAX_SIZE_ARRAY * sizeof(depend_t));
	//if cmd->input != NULL
		//new_dep->input = cmd->input
	//else
		//new_dep->input = cmd->u.word[], where word is not an option
	//if input is in output list (most recent output)
		//new_dep->dependency = old_dep->pid; 
	//add to dependency list
	return;
}

// increase array size for dep node input files
void grow_in_ptr(dep_node_t dn)
{
        dep_node_in_array_size += DEP_NODE_IO_ARRAY_GROW;
        char **temp = checked_realloc(dn->in, dep_node_in_array_size * sizeof(char**));
        if (temp == NULL)
                error(1, 0, "Error with memory reallocation\n");
        dn->in = temp;
}

// increase array size for dep node output files
void grow_out_ptr(dep_node_t dn)               
{
        dep_node_out_array_size += DEP_NODE_IO_ARRAY_GROW;
        char **temp = checked_realloc(dn->out, dep_node_out_array_size * sizeof(char**));         
        if (temp == NULL)
                error(1, 0, "Error with memory reallocation\n");
        dn->out = temp;
}

// add input and output from redirection to dep node
void add_in_out(command_t c, dep_node_t dn)
{
    if (c->input != NULL)
    {
        dn->in[dn->numOfInput] = c->input;
        dn->numOfInput++;
        if (dn->numOfInput == dep_node_in_array_size)
            grow_in_ptr(dn);
    }
    if (c->output != NULL)
    {
        dn->out[dn->numOfOutput] = c->output;
        dn->numOfOutput++;
	if (dn->numOfOutput == dep_node_out_array_size)
	    grow_out_ptr(dn);
    }
}

// for AND, OR, SEQUENTIAL, PIPE
void addingSequentialToDN(command_t c, dep_node_t dn)
{
    make_dependency_list(c->u.command[0], dn);
    make_dependency_list(c->u.command[1], dn);
}

void addingSubshellToDN(command_t c, dep_node_t dn)
{
    add_in_out(c, dn); // add input files from redirection if any
    make_dependency_list(c->u.subshell_command, dn);
}


void addingSimpleToDN(command_t c, dep_node_t dn)
{
    add_in_out(c, dn); // add input and output files from redirection if any
    int numOfWords = 0;
    while(c->u.word[numOfWords] != NULL) // get other input files
    {
        if (numOfWords = 0) // skip the first word which is the command
	{    
	    numOfWords++;
	    continue;
	}
	if(c->u.word[numOfWords][0] == '-') // ignore options
	{
	    numOfWords++;
	    continue;
	}
        dn->in[dn->numOfInput] = c->u.word[numOfWords];
        dn->numOfInput++;
	numOfWords++;
    }
}

void execute_time_travel (command_t c)
{
   //make_dependency_list(c);
   //while (there are still runnable commands) 
	//for all nodes
		//if curr_node->dependency != NULL
			//continue; 
		//fork()
		//if child
			//execute_cmd()
			//exit
		//else
			//save child pid
	//wait() = pid
	//update dep, then run current command
	return;
}

// set default values for new dep node
void init_dep_node(dep_node_t dn)
{
	dn->in = checked_malloc(dep_node_in_array_size * sizeof(char*));
	if (dn->in == NULL)
		error(1, 0, "Error with memory allocation\n");
	dn->out = checked_malloc(dep_node_out_array_size * sizeof(char*));
        if (dn->out == NULL)
                error(1, 0, "Error with memory allocation\n");
	dn->numOfInput = 0;
	dn->numOfOutput = 0;
        // can only have as many dependencies as there are prev dep nodes
	dn->dependency = checked_malloc(numOfDepNodes * sizeof(dep_node_t*)); 
	if (dn->dependency == NULL)
		error(1, 0, "Error with memory allocation\n");
	dn->numOfDependencies = 0;
	dn->isDone = false;
}

// create dependency list for time traveling
void setup_new_dep_node(void)
{
        dep_node_in_array_size = DEP_NODE_IO_ARRAY_GROW; // reset IO memory allocations
        dep_node_out_array_size = DEP_NODE_IO_ARRAY_GROW;
        if(numOfDepNodes == 0) // haven't created Dependency Node Tree yet
        {        
            dep_access = checked_malloc(dep_node_array_size * sizeof(dep_node_t));
            if (dep_access == NULL)
                error(1, 0, "Error with memory allocation\n");
        }
        if(numOfDepNodes == dep_node_array_size) // need to grow array for more dep nodes
        {
            dep_node_array_size += DEP_NODE_ARRAY_GROW;
            dep_node_t temp = checked_realloc(dep_access, dep_node_array_size * sizeof(dep_node_t));
            if (temp == NULL)
                error(1, 0, "Error with memory reallocation\n");
            dep_access = temp;
        }
        dep_access[numOfDepNodes] = checked_malloc(sizeof(struct dep_node));
        if (dep_access[numOfDepNodes] == NULL)
            error(1, 0, "Error with memory allocation\n");
        init_dep_node(dep_access[numOfDepNodes]); // initialize dep node 
        numOfDepNodes++;
}

// check previous dep nodes for input-output dependencies
void check_for_prev_dependencies(dep_node_t dn)
{
    int depNodeCount = numOfDepNodes-1; // counting backwards through all prev dep nodes
    while (depNodeCount > 0)
    {
        int inputCount = 0;
	int outputCount = 0;
	while (inputCount < dn->numOfInput)
	{
	    while (outputCount < dep_access[depNodeCount]->numOfOutput)
	    {
	        if (strcmp(dn->in[inputCount], dep_access[depNodeCount]->out[outputCount]) == 0)
		{
		    dn->dependency[dn->numOfDependencies] = dep_access[depNodeCount];
		    dn->numOfDependencies++;
		}
	        outputCount++;
	    }
	    inputCount++;
	}
	depNodeCount--;
    }
}


void
execute_command(command_t c, bool time_travel)
{
   /*FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  
    */
	if (time_travel == false)
	{
		execute_switch(c);
	}
	else
	{
		setup_new_dep_node();
		// the dep list needs to be created for all command streams before execute_time_travel can be run..
		make_dependency_list(c, dep_access[numOfDepNodes]);
	        check_for_prev_dependencies(dep_access[numOfDepNodes]);	
		// add some kind of loop to go through the streams again
		// to run the time travel
				
		//execute_time_travel(c);
	}
}
