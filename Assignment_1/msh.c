/*
Matthew Irvine
1001401200
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#define MAX 1024

//This was a quality of life decision ( to make a string struct)
//I didnt want to have to contemplate 2d char arrays so I used this instead
//basically does the same thing.
typedef struct Str {
	char name[MAX];
}String;

char** parseCmd(char* cmd)
{
	char** args = (char**)malloc(MAX);
	char* arg = (char*)malloc(MAX);
	char** ptr = args;
	//uses strtok to parse the string into the individual arguments
	arg = strtok(cmd, " ");
	strcat(arg, "\0");
	while (arg != NULL)
	{
		//pointer manipulation if found to be easier than iterative
		//the spot ptr points to is now arg, set arg to the next argument
		//advance ptr to next position, repeat.
		*ptr = arg;
		arg = strtok(NULL, " ");
		strcat(arg, "\0");
		ptr++;
	}

	free(arg);
	return args;
}

int doit(char** cmd,String str[], pid_t pids[], int psize, String unparsed, int pos)
{
	//some setup for this funciton
	char** ptr = cmd;
	int fail;
	int status = 0;
	pid_t pid = fork();

	if (pid == -1)
	{
		perror("fork");
		exit(-1);
	}

	if (pid == 0)
	{
		//does any command in the bin files, or the current directory
		fail = execvp(*ptr, ptr);
		if (errno == ENOENT || fail == -1)
		{
			char* failure = (char*)malloc(MAX);
			strcat(failure, *ptr);
			strcat(failure, ": Command Not Found.");
			printf("%s\n", failure);
			free(failure);
			exit(1);
		}
	}
	else
	{
		wait(&status);
	}
	if (WEXITSTATUS(status) == 0)
	{
		strcpy(str[pos].name, unparsed.name);
		//Add pid id to list.
		for (int i = 0; i < psize; i++)
		{
			if (pids[i] == -1)
			{
				pids[i] = pid;
				break;
			}
		}
		return 1;
	}
	else
	{
		//if it fails send by a 2 so that main knows to keep c the same.
		return 2;
	}
}

int main()
{
	//lets start by printing out something on every line.
	//int cmd is used to control the do-while loop.
	int cmd = 0;
	char ** p_cmd;
	char * strcmd = malloc(MAX);	//see if you can find the max terminal length
	char* msh = (char*)malloc(5);
	

	strcpy(msh, "msh>");
	int list_size = 15;
	String str[list_size];
	pid_t pids[list_size];
	memset(pids, -1, sizeof(pids));
	int c = 0;
	//literally just a whole bunch of setup, nothing really special
	//except, perhaps, the String str[15].
	//but everything else is pretty standard, I think.

	do
	{
		//used as the list reset for both showpids and history
		if (c == 15)
			c = 0;

		printf("%s", msh);
		fflush(stdout);
		fgets(strcmd, MAX, stdin);
		strcmd[strcspn(strcmd, "\n")] = 0;	//don't want newlines I want null terminators
		String cpy;
		strcpy(cpy.name, strcmd);

		///BUILT IN FUNCITONS

		//I do this here because I want to change what strcmd is before it is sent to 
		//be parsed and then executed.
		//first checks if ! occurs in the command
		if (strpbrk(strcmd, "!") != NULL)
		{
			const char nums[] = "0123456789";

			//Then checks if a number is also associated
			char* tmp = strpbrk(strcmd, nums);	

			//if not then it isn't a command that we can do.
			if (tmp == NULL)					
			{
				printf("Not an option to chose from.\n");
				continue;
			}
			//Otherwise change the strings to ints
			int digits = atoi(tmp);				

			//if the int isn't in range
			//Or if the history position is blank
			//then leave.
			if ((digits < 0 || digits > 14) || strcmp(str[digits].name, "") == 0)	
			{
																					
				printf("Command not in history.\n");
				continue;
			}
			else	//othersize overwrite strcmd with history command.
			{
				strcpy(strcmd, str[digits].name);
				strcpy(cpy.name, strcmd);
			}	
		}

		//prevents program halt if only whitespace is given.
		if (isspace(*strcmd))				
		{
			cmd = 1;
			continue;
		}

		//this figures out if the command is a quit or exit command,
		//passes back zero to end the program
		if (strcmp(strcmd, "quit") == 0 || strcmp(strcmd, "exit") == 0)
			exit(0);

		// it is okay that this comes before the strcpy(str[c].name, strcmd)
		// because c doesn't increment, thus the spot where whitespace would go is
		// overwritten.
		if (strcmp(strcmd, "") == 0)		
			continue;						
											
		p_cmd = parseCmd(strcmd);

		//the following functions are those that I do not want to create child processes for...
		if (strcmp(*p_cmd, "history") == 0)
		{
			strcpy(str[c].name, *p_cmd);

			//since doit function isn't called this doesn't get added in showpids
			//so I do it manually.
			pids[c] = getpid();
			int count = 0;

			//as long as it ain't blank print it.
			while (strcmp(str[count].name, "") != 0)
			{
				printf("%d:\t%s\n", count, str[count].name);
				count++;
				if (count == list_size)
					break;
			}
			
			c++;
			continue;
		}

		if (strcmp(*p_cmd, "showpids") == 0)
		{
			strcpy(str[c].name, *p_cmd);

			//since doit function isn't called this doesn't get added in showpids
			//so I do it manually.
			pids[c] = getpid();
			//as long as it ain't a -1 print the number.
			for (int i = 0; i < list_size; i++)
			{
				if (pids[i] == -1)
					break;
				printf("%d:\t%d\n", i, pids[i]);
			}
			c++;
			continue;
		}

		//checks for the cd command, then uses chdir() to change directory,
		//after properly concatenating the parsed string
		if (strcmp(*p_cmd, "cd") == 0)
		{
			strcat(str[c].name, *p_cmd);	//need this in history
			strcat(str[c].name, " ");
			char* dir = (char*)malloc(MAX);
			
			//don't want to include cd in the name of the directory
			p_cmd++;	
			int counter = 0;
			
			//concatenates parsed string
			do
			{
				//adds the space between parsed strings, except before the first string
				if (counter > 0)	
				{
					strcat(str[c].name, " ");
					strcat(dir, " ");
				}
				strcat(dir, *p_cmd);
				strcat(str[c].name, *p_cmd);
				p_cmd++;
				counter++;
			} while (*p_cmd != NULL);
			
			//just a reset, in case I need it. Also accounts for previous offset of 1
			p_cmd = p_cmd - (c+1);	
			strcat(dir, "/");
			if (chdir(dir) == -1)
			{
				dir[strcspn(dir, "/")] = 0;
				perror(dir);
			}
			else
				cmd = 1;
			free(dir);
			//need to make sure cd gets recorded on history
			c++;		
			continue;
		}

		///BUILT IN FUNCITONS END

		cmd = doit(p_cmd,str,pids,list_size,cpy,c);//JUST DO IT

		//I need to be sure that any function that fails doesn't increment c
		//otherwise the history function will not work.
		if (cmd != 2)
			c++;
	} while (cmd > 0);
	free(msh);
	free(p_cmd);
	free(strcmd);
	return 0;
}
