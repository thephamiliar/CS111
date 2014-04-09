// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
#include <string.h>
#include "alloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_SIZE_ARRAY 1024
bool DEBUG = false;

bool isWordChar(char c) // is alpha-numeric or ! % + , - . / : @ ^ _
{
    return isalpha(c) || isdigit(c)
    || (c == '!') || (c == '%') || (c == '+') || (c == ',')
    || (c == '-') || (c == '.') || (c == '/') || (c == ':')
    || (c == '@') || (c == '^') || (c == '_');
}

bool isSpecial(char c) // is ; | && || < >
{
    return (c == ';') || (c == '|') || (c == '&') || (c == '<') || (c == '>');
}

bool parenCountCheck(char c, int *parenCount, const int lineNum) // check for paren, and if number paren is valid
{
    if (c == '(')
    {
        (*parenCount)++;
        return true;
    }
    else if (c == ')')
    {
        (*parenCount)--;
        if ((*parenCount) < 0)
        {
            fprintf(stderr, "%d: Error, no matching open parenthesis\n", lineNum);
            exit(1);
        }
        return true;
    }
    return false;
}

bool isSyntaxGood(char *linePos, int *parenCount, const int lineNum)
{
    int i = 0;
    if (linePos[0] == '\0')
        return true;
    while ((linePos[i] == ' ') || (linePos[i] == '\t'))
        i++;
    char c = linePos[i];
    if (!isWordChar(c) && !parenCountCheck(c, parenCount, lineNum))
    {
        fprintf(stderr, "%d: Error, invalid line start\n", lineNum);
        exit(1);
    }
    i++;
    while(linePos[i] != '\0')
    {
        c = linePos[i];     //speed up accesses in rest of function
        char b = linePos[i-1];
        char d = linePos[i+1];
        
        if (c == ' ' || c == '\t')
            i++;
        else if (isSpecial(c))
        {
            if (isSpecial(b)) //error: two special characters unless &, |
            {
                fprintf(stderr, "%d: Syntax error\n", lineNum);
                exit(1);
            }
            else if ((c == '&' && d == '&') || (c == '|' && d == '|'))
                i+=2;
            else if (c == '&') // single & is error
            {
                fprintf(stderr, "%d: Syntax error\n", lineNum);
                exit(1);
            }
            else
                i++;
        }
        else if (parenCountCheck(c, parenCount, lineNum))
            i++;
        else if (isWordChar(c))
            i++;
        else    // invalid character
        {
            fprintf(stderr, "%d: Syntax error\n", lineNum);
            exit(1);
        }
    }
    while ((i > 0) && (linePos[i-1] == ' ' || linePos[i-1] == '\t')) //get rid of white space at the end
        i--;
    if (i > 0 && (linePos[i-1] == '<' || linePos[i-1] == '>')) //error: line can't end in < or >
    {
        fprintf(stderr, "%d: Error, cannot end line in < or >\n", lineNum);
        exit(1);
    }
    return true;
}

//HELPER FUNCTIONS
int get_next_token (char *s, int* index, enum command_type opStack[], int* opSize, command_t* cmdStack, int* cmdSize);
void tokenizer (char* s, enum command_type opStack[], int* opSize, command_t* cmdStack, int* cmdSize);
command_t make_simple_cmd (char* word);
command_t make_special_cmd (command_t left, command_t right, enum command_type type);
command_t make_subshell_cmd (command_t cmd);
command_t make_tree (enum command_type opStack[], int* opSize, command_t* cmdStack, int* cmdSize);

/* REALLOCATE MEMORY */

/* GET_NEXT_TOKEN: retrieves next tokens and adds to stack assuming syntax is valid*/
int get_next_token (char *s, int* index, enum command_type opStack[], int* opSize, command_t* cmdStack, int* cmdSize)
{
    char* word = checked_malloc (MAX_SIZE_ARRAY*sizeof(char));
    int sizeOfWord = 0;
    while (s[(*index)] != '\0')
    {
	if (s[(*index)] == '|')
        {
	    int i = (*index)+1;	    
	    if (s[i] == '|')
	    {
	    	//push word onto cmd stack as simple command
		if (sizeOfWord > 0)
		{
	       		word[sizeOfWord] = '\0'; 
	    		command_t cmd = make_simple_cmd(word);
	    		cmdStack[(*cmdSize)] = cmd;
			 *cmdSize= (*cmdSize)+1;
		}
	    	//push '||' onto commands stack
	    	opStack[(*opSize)] = OR_COMMAND;
	    	(*index) = (*index)+1;
	    	*opSize = (*opSize)+1;
	    	free(word);
	    } else {	
		if (sizeOfWord > 0)
		{
	   	word[sizeOfWord] = '\0';
	    	command_t cmd = make_simple_cmd(word);
	    	cmdStack[(*cmdSize)] = cmd;
	  	*cmdSize= (*cmdSize)+1;
		}
	    	//push s[index] onto op stack
	    	opStack[(*opSize)] = PIPE_COMMAND;
	    	*opSize = (*opSize)+1;
	    	free(word);
	    	
	    }
	    return 1;
	}
	else if (s[(*index)] == '(')
        {
	    if(sizeOfWord > 0)
	    {
	   	 //push word onto cmd stack as simple command
	        word[sizeOfWord] = '\0';
	        command_t cmd = make_simple_cmd(word);
	    	cmdStack[(*cmdSize)] = cmd;
	        *cmdSize = (*cmdSize+1);
	    }
	
	    //push s[index] onto op stack
	    opStack[(*opSize)] = SUBSHELL_COMMAND;
	    *opSize = (*opSize)+1;
	    free(word);
    	    
	    return 1;
	}
	else if (s[(*index)] == ')')
	{
	    if (sizeOfWord > 0)
	    {
	   	 //push word onto cmd stack as simple command
	   	 word[sizeOfWord] = '\0';
	   	 command_t cmd = make_simple_cmd(word);
	   	 cmdStack[(*cmdSize)] = cmd;
		 *cmdSize= (*cmdSize)+1;
	    }
	    //push s[index] onto op stack
	    opStack[(*opSize)] = SUBSHELL_COMMAND2;
	    *opSize = (*opSize)+1;
	    free(word);
    	    return 1;

	}
	else if (s[(*index)] == ';')
	{
	    if (sizeOfWord > 0)
	    {
	    	//push word onto cmd stack as simple command
	    	word[sizeOfWord] = '\0';
	    	command_t cmd = make_simple_cmd(word);
	    	cmdStack[(*cmdSize)] = cmd;
	        *cmdSize= (*cmdSize)+1;
	    }
	    //push s[index] onto op stack
	    opStack[(*opSize)] = SEQUENCE_COMMAND;
	    *opSize = (*opSize)+1;
	    free(word);
    	    return 1;
	}
	else if (s[(*index)] == '&')
	{
	    if (sizeOfWord > 0)
	    {
	   	 //push word onto cmd stack as simple command
	    	word[sizeOfWord] = '\0';
	    	command_t cmd = make_simple_cmd(word);
	    	cmdStack[(*cmdSize)] = cmd;
	        *cmdSize= (*cmdSize)+1;
	    }
	    //push '&&' onto stack
	    opStack[(*opSize)] = AND_COMMAND;
	    *opSize = (*opSize)+1;
	    free(word);
	    (*index) = (*index)+1;
	    return 1;
	}
	else
	{
	    word[sizeOfWord]=s[(*index)];
	    sizeOfWord++;
	}
	(*index) = (*index)+1;
    }
    free(word);
    return 0;
}
/* PRINT COMMANDS */
void print (command_t cmd)
{
    	    if (cmd->type == SIMPLE_COMMAND)
		printf("CMD: %s\n", cmd->u.word[0]);
	    else if (cmd->type == SUBSHELL_COMMAND)
	    {
		printf("SUBSHELL COMMAND\n");
		print(cmd->u.subshell_command);
	    }
	    else
	    {
		printf("SPECIAL CMD A: ");
		print(cmd->u.command[0]);
		printf("SPECIAL CMD B: ");
		print(cmd->u.command[1]);

	    }
}
/* TOKENIZER: gets tokens and adds to stacks*/
void tokenizer (char* s, enum command_type opStack[], int* opSize, command_t* cmdStack, int* cmdSize)
{
    int index = 0;

    int token = get_next_token(s, &index, opStack, opSize, cmdStack, cmdSize);

    while (token== 1)
    {

	index++;
        enum command_type op1 = opStack[(*opSize)-1];
	enum command_type op2 = opStack[(*opSize)-2];
	//if op token check is higher precedence
	if (op1 == SUBSHELL_COMMAND2)
    	{
	    command_t cmd = cmdStack[(*cmdSize)-1];
	    *opSize = (*opSize)-1; *cmdSize = (*cmdSize)-1;
	    //pop until opening parentheses = cmd
	    while (op1 != SUBSHELL_COMMAND)
	    {
		cmd = make_special_cmd(cmdStack[(*cmdSize)-1], cmd, opStack[(*opSize)-1]);
		*opSize = (*opSize)-1; *cmdSize = (*cmdSize)-1;
		op1 = opStack[(*opSize)-1];
	    }
	    //push special command onto cmd stack
	    command_t new_cmd = make_subshell_cmd(cmd);
	    cmdStack[(*cmdSize)] = new_cmd;
	    *opSize = (*opSize)-1; *cmdSize = (*cmdSize)+1;
    	}
	else if (op1 == PIPE_COMMAND && op1 > op2)
	{
	    int i = index;
	    if (s[i] != '\0')
	    {
	    //pop pipe and top of cmd stack = left
	    *opSize = (*opSize)-1; 
	    token = get_next_token(s, &index, opStack, opSize, cmdStack, cmdSize);
	    //pop top of cmd stack = right
	    command_t new_cmd = make_special_cmd(cmdStack[(*cmdSize)-2], cmdStack[(*cmdSize)-1], PIPE_COMMAND);
	    *cmdSize = (*cmdSize)-2;
	    //push special command onto cmd stack
	    cmdStack[(*cmdSize)] = new_cmd;
	    *cmdSize = (*cmdSize)+1;
	    continue;
	    }
	    else
	    	break;
	}
	
	token = get_next_token(s, &index, opStack, opSize, cmdStack, cmdSize);
    }
}

/* MAKE_SIMPLE_COMMAND*/
command_t make_simple_cmd (char* word)
{
    if(DEBUG)
	printf("Making simple command... %s\n", word);
    command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
    new_cmd->type = SIMPLE_COMMAND;
    new_cmd->status = -1;
    new_cmd->input = NULL;
    new_cmd->output = NULL;
    new_cmd->u.word = (char**) checked_malloc(MAX_SIZE_ARRAY*sizeof(char*));
    int i = 0; int numOfWords = 0; int numOfChars = 0;
    char *curr_word = (char*) checked_malloc(MAX_SIZE_ARRAY*sizeof (char));
    while (word[i] != '\0')
    {
	if ((word[i] == ' ' || word[i] == '\t' || word[i] == '<' || word[i] == '>') && numOfChars != 0)
	{
	    new_cmd->u.word[numOfWords] = curr_word;
            curr_word = (char*) checked_malloc(MAX_SIZE_ARRAY*sizeof (char));
	    numOfWords++;
	    numOfChars=0;
	}
	if (word[i] == '<')
	{
	    i++;
	    while (word[i] != '<' && word[i] != '>' && word[i] != '\0')
	    {
		if (word[i] != ' ' && word[i] != '\t')
		{
		     curr_word[numOfChars] = word[i];
		     numOfChars++;
		}
		i++;
	    }
	    //input
	   new_cmd->input = curr_word; 
           curr_word = (char*) checked_malloc(MAX_SIZE_ARRAY*sizeof (char));
	   numOfChars=0;
	   continue;
	}
	if (word[i] == '>')
	{
	    i++;
	    while (word[i] != '<' && word[i] != '>' && word[i] != '\0')
	    {
		if (word[i] != ' ' && word[i] != '\t')
		{
		     curr_word[numOfChars] = word[i];
		     numOfChars++;
		}
		i++;
	    }
	    //output
	   new_cmd->output = curr_word; 
           curr_word = (char*) checked_malloc(MAX_SIZE_ARRAY*sizeof (char));
	   numOfChars=0;
 	   continue;
	}
	else
	{
	    if (word[i] != ' ' && word[i] != '\t')
	    {
	    	curr_word[numOfChars]=word[i];
	    	numOfChars++;
	    }
	}
	i++;
    }
    if (numOfChars == 0)
    {
	free(curr_word);
    }
    else
    {
        new_cmd->u.word[numOfWords] = curr_word;
    }
    if (DEBUG)
	printf("Exiting simple command...\n");
    return new_cmd;
}

/* MAKE_SPECIAL_COMMAND: &&, or, pipe, sequential */
command_t make_special_cmd (command_t left, command_t right, enum command_type type)
{
    if(DEBUG)
	printf("Making special command...\n");
    command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
    new_cmd->type = type;
    new_cmd->status = -1;
    new_cmd->input = NULL;
    new_cmd->output = NULL;
    new_cmd->u.command[0] = left;
    new_cmd->u.command[1] = right;
    if(DEBUG)
	printf("Exiting special command...\n");
    return new_cmd;
}

/* MAKE_SUBSHELL_COMMAND */
command_t make_subshell_cmd(command_t cmd)
{
    if(DEBUG)
	printf("Making subshell command...\n");
    command_t new_cmd = (command_t) checked_malloc(sizeof(struct command));
    new_cmd->type = SUBSHELL_COMMAND;
    new_cmd->status = -1;
    new_cmd->input = NULL;
    new_cmd->output = NULL;
    new_cmd->u.subshell_command = cmd;
    if(DEBUG)
	printf("Exiting special command...\n");

    return new_cmd;
}

/* MAKE_TREE: make stream from stacks, returns root of tree */
command_t make_tree (enum command_type opStack[], int* opSize, command_t* cmdStack, int* cmdSize)
{
        enum command_type op1 = opStack[(*opSize)-1];
	enum command_type op2 = opStack[(*opSize)-2];
	//if op token check is higher precedence
	if (op1 == PIPE_COMMAND && op1 > op2)
	{
	    //pop pipe and top of cmd stack = left
	    *opSize = (*opSize)-1; 
	    //pop top of cmd stack = right
	    command_t new_cmd = make_special_cmd(cmdStack[(*cmdSize)-2], cmdStack[(*cmdSize)-1], PIPE_COMMAND);
	    *cmdSize = (*cmdSize)-2;
	    //push special command onto cmd stack
	    cmdStack[(*cmdSize)] = new_cmd;
	    *cmdSize = (*cmdSize)+1;
	}
    //have to pop from the front of the stacks!!!!!
    command_t cmd1, cmd2; int c1 = 0; int c2 = 1; int op = 0;
    while(*opSize > op)
    {
	    cmd1 = cmdStack[c1];
	    cmd2 = cmdStack[c2];
	    c1++; c2++;
    	    cmd2 = make_special_cmd(cmd1, cmd2, opStack[op]);
	    cmdStack[c1] = cmd2;
	    op++;
    }
    return cmdStack[c1];
}
command_stream_t init_cmd_stream()
{
    command_stream_t cmdStream = (command_stream_t) checked_malloc(sizeof(struct command_stream));
    cmdStream->root = NULL;
    cmdStream->last = NULL;
    return cmdStream;
}
/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */


  //TOKENIZE/PARSE SCRIPT & ERROR CHECKING
  //allocate dynamic array
  char *stream; int line=1; int max_size = MAX_SIZE_ARRAY;
  stream = (char *) checked_malloc(MAX_SIZE_ARRAY*sizeof(char));
  int sizeOfStream = 0;
  //read chars line by line until EOF
  int c; int parenCount = 0; int newLine = 0;
  command_t *cmdStack;
  int opSize = 0; int cmdSize = 0; bool endToken = false;
  enum command_type opStack [MAX_SIZE_ARRAY];
  cmdStack =  (command_t*) checked_malloc(MAX_SIZE_ARRAY*sizeof(struct command_t*)); 
  command_stream_t cmdStream = init_cmd_stream();

  while ((c=get_next_byte(get_next_byte_argument)) != EOF)
  { 
	if (c == '#' && sizeOfStream == 0)
	{
	    if (DEBUG)
	 	printf("LINE %d: \n", line);
		while ((c != EOF) && (c != '\n'))
		{
			c=get_next_byte(get_next_byte_argument);
			continue;
		}
		line++;
	     newLine = 0;
 	}
	else if (c == '\n') //CHECK FOR DOUBLE NEWLINES, HAVE TO MAKE NEW STACKS 
	{
	    newLine++;
	    //check for memory allocation
	    if (sizeOfStream == max_size)
	    {
		max_size += 1024;
		checked_realloc(stream, max_size*sizeof(char));
	    }
	    if (newLine > 1)
	    {
 		 if (endToken)
		 {
			continue;
		  }
		//remove top of opStack
		opSize--;
		cmd_node_t node = (cmd_node_t) checked_malloc(sizeof(struct cmd_node));
 
		//make tree
		command_t root = make_tree(opStack, &opSize, cmdStack, &cmdSize);
	
  		node->next = NULL;
		node->c = root;
		if (!cmdStream->root)
		{
		    cmdStream->root = node;
		    cmdStream->last = node;
		}
		else
		{
		   cmdStream->last->next = node;
		   cmdStream->last = node; 
		}
		
		//reset stacks
		opSize = 0; cmdSize = 0; endToken = false;
		cmdStack =  (command_t*) checked_malloc(MAX_SIZE_ARRAY*sizeof(struct command_t*)); 
//	   	printf("COMMAND STREAM\n");print(root);
	    }
	    else
	    {
		stream[sizeOfStream] = '\0';

		//pass stream and parenCount into syntax checker
		isSyntaxGood(stream, &parenCount, line);
		if (!isSpecial(stream[sizeOfStream-1]) && (stream[sizeOfStream-1] != '('))
		{
			stream[sizeOfStream] = ';';
			stream[sizeOfStream+1] = '\0';
			endToken = false;
		}
		else
			endToken = true;
		//tokenize function
		tokenizer (stream, opStack, &opSize, cmdStack, &cmdSize);

		//free(stream);
		stream = (char *) checked_malloc(MAX_SIZE_ARRAY*sizeof(char));
		sizeOfStream = 0;
	    }
	    line++;
	}
	else
	{
	    //check for memory allocation
	    if (sizeOfStream == max_size)
	    {
		max_size += 1024;
		checked_realloc(stream, max_size*sizeof(char));
	    }
		stream[sizeOfStream] = c;
		sizeOfStream++;
		newLine = 0;
	}
  }

  if (endToken)
  {
	fprintf(stderr, "%d: End of file has special token\n", line);
	exit(1);
  }

  opSize--;
  cmd_node_t node = (cmd_node_t) checked_malloc(sizeof(struct cmd_node));

  //make tree
  command_t root = make_tree(opStack, &opSize, cmdStack, &cmdSize);
  node->next = NULL;
  node->c = root;
  if (!cmdStream->root)
  {
	cmdStream->root = node;
	cmdStream->last = node;
  }
  else
  {
	cmdStream->last->next = node;
	cmdStream->last = node; 
  }
  //printf("COMMAND STREAM\n"); print(root);
 // free(stream); 

  if (parenCount != 0)
  {
	fprintf(stderr, "%d: Missing parentheses\n", line);
	exit(1);	
  }

  // free(cmdStack);
  return cmdStream;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  if (s->root)
  {
	command_t cmd = s->root->c;
	cmd_node_t tmp = s->root;
	s->root = s->root->next;
	free(tmp);
	return cmd;
  } 
  return NULL;
}
