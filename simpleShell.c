
/****************            Program to create a Simple Unix Shell           *****************
 ****************
 **************** Brief explanation about the program : The main function accepts 
 **************** input, breaks it into individual commands if entered by using ';'
 **************** then each individual command is sent to parse() function to
 **************** split it into tokens,  after that execute() function is called
 **************** where in the actual command 'execvp' is called to run the command
 **************** I have chosen 'execvp' though there are five other exec commands
 **************** because of its simplicity to use.  The 'execl', 'execle' and 'execlp'
 **************** functions require the arguments as a separate list whereas 'execv',
 **************** 'execve' and 'execvp' require an array name which contains all the
 **************** array of arguments.  Again within these three, 'execvp' searches
 **************** the environment path to locate the command and we need not write
 **************** code to search.
 ***************/


#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>


void execute(char **, int, char **);

void handle_signal(int);
int parse(char *, char **, char **, int *);
void trim(char *);

#define CMD_LENGTH 80
#define SIMPLE 			1	
#define REDIRECT_OUT 	2
#define REDIRECT_IN 	3
#define PIPELINE 		4
#define BACKGROUND		5
#define APPEND_OUT		6

#define SEPARATORS "\n\t "

typedef void (*sighandler_t)(int);



/****************      Main Function                          *****************
 ****************
 **************** In this function, home dir is set, inside while loop cur dir 
 **************** is set, and getline() function call takes input from the
 **************** user where full command line is entered.  This is broken into
 **************** individual commands when ; is used and separately processed
 ***************/


int main(int argc, char *argv[])
{
	int i, mode = SIMPLE, cmdArgc;
	size_t len = CMD_LENGTH;
	char *cpt, *command, *fullcommand,*cmdArgv[CMD_LENGTH], *extCommand, *rwFile = NULL;
	command = (char*)malloc(sizeof(char)*CMD_LENGTH);
	extCommand = (char*)malloc(sizeof(char)*CMD_LENGTH);
	fullcommand = (char*)malloc(sizeof(char)*CMD_LENGTH);
	
	char curDir[100], homDir[100];
	char *prompt="MyShell";		// Initial Prompt

	signal(SIGINT, SIG_IGN);	// Disables Ctrl-c
	signal(SIGQUIT, SIG_IGN);	// Disables Ctrol-\
	signal(SIGTSTP, SIG_IGN);	// Disables Ctrol-z
	signal(SIGINT, handle_signal);	// Signal handler routine
	signal(SIGQUIT, handle_signal);
	signal(SIGTSTP, handle_signal);
	
	getcwd(homDir,100);		// To set initial prompt to Home dir
	while(1)
	{
		mode = SIMPLE;
		getcwd(curDir, 100);	// To repeatedly set current dir
		

		printf("%s [%s] ", curDir, prompt);
		getline( &fullcommand, &len, stdin);	// Storing the entire command line
		if(strcmp(fullcommand, "exit\n") == 0)
			exit(0);
		if(strcmp(fullcommand, "\n") == 0)
		continue;

		cpt=command;
		while(*fullcommand !='\n')
		{
			*cpt++ = *fullcommand++;
			if(*fullcommand == ';' || *fullcommand == '&')		// Breaking the commandline into
				{			// separate commands and process
				if (*fullcommand == '&')
					*cpt++ = *fullcommand++;
				else
					*fullcommand++;		// them one by one
				*cpt='\0';
			if (index(command,'*')!=NULL || index(command,'?')!=NULL)
			{								// if wildcard chars like * and ? are
				myGlob(command, &extCommand);				// are entered, then the command is passed
				strcpy(command,extCommand);				// to myGlob function where they are
			}								// expanded and sent back in the string 'extCommand'
				cmdArgc = parse(command, cmdArgv, &rwFile, &mode);
		if(strcmp(*cmdArgv, "cd") == 0  && cmdArgc == 2)
			{				
			chdir(cmdArgv[1]);		// Change to the dir required
			}
		else 
		if(strcmp(*cmdArgv, "cd")==0 && cmdArgc == 1)
			{
			chdir(homDir);			// Change to home dir
			}
		else
			if(strcmp(*cmdArgv, "prompt")==0)
			{
			prompt=strdup(cmdArgv[1]);	// Changing Prompt
			}
		else
			{
			execute(cmdArgv, mode, &rwFile);
			}
		bzero(extCommand,CMD_LENGTH);
		//		execute (cmdArgv, mode, &rwFile);
		//		bzero(extCommand,CMD_LENGTH);
				cpt=command;
		mode = SIMPLE;
				}
		}
		*cpt='\0';
		if (index(command,'*')!=NULL || index(command,'?')!=NULL)		// same action as mentioned above
		{
		myGlob(command, &extCommand);
		strcpy(command,extCommand);
		}
		cmdArgc = parse(command, cmdArgv, &rwFile, &mode);

		if(strcmp(*cmdArgv, "cd") == 0  && cmdArgc == 2)
			{				
			chdir(cmdArgv[1]);		// Change to the dir required
			}
		else 
		if(strcmp(*cmdArgv, "cd")==0 && cmdArgc == 1)
			{
			chdir(homDir);			// Change to home dir
			}
		else
			if(strcmp(*cmdArgv, "prompt")==0)
			{
			prompt=strdup(cmdArgv[1]);	// Changing Prompt
			}
		else
			{
			execute(cmdArgv, mode, &rwFile);
			}
		bzero(extCommand,CMD_LENGTH);
	}
	return 0;
}




/****************      Function to parse the command line       *****************
 ****************
 **************** The first param of the func takes the commandline and 
 **************** passes the argument list, input/out file name, and
 **************** Input Redirection/Output Redirection/Background process
 **************** etc. through remaining three parameters respectively
 ***************/ 


int parse(char *command, char *cmdArgv[], char **rwFile, int *cmdMode)
{
	int cmdArgc = 0, terminate = 0;
	char *cmdLine = command;

	while((*cmdLine == ' ' || *cmdLine == '\t' || *cmdLine == '\n') && terminate == 0)
	{
		*cmdLine = '\0';
		cmdLine++;
	}
	while(*cmdLine != '\0' && terminate == 0)
	{
		*cmdArgv = cmdLine;
		cmdArgc++;
		while(*cmdLine != ' ' && *cmdLine != '\t' && *cmdLine != '\0' && *cmdLine != '\n' && terminate == 0)
		{
			switch(*cmdLine)
			{
				case '&':
					*cmdMode = BACKGROUND;
					*cmdArgv = '\0';
					terminate = 1;
					break;
				case '>':
					*cmdMode = REDIRECT_OUT;
					*cmdArgv = '\0';
					cmdLine++;
					if(*cmdLine == '>')
					{
						*cmdMode = APPEND_OUT;
						cmdLine++;
					}
					while(*cmdLine == ' ' || *cmdLine == '\t')
						cmdLine++;
					*rwFile = cmdLine;		// Output file name
					trim(*rwFile);
					terminate = 1;			// Command line processing is
					break;				// stopped once we encounter
				case '<':				// special chars like >,<,|
					*cmdMode = REDIRECT_IN;
					*cmdArgv = '\0';
					cmdLine++;
					while(*cmdLine == ' ' || *cmdLine == '\t')
						cmdLine++;
					*rwFile = cmdLine;		// Input file name
					trim(*rwFile);
					terminate = 1;
					break;
				case '|':
					*cmdMode = PIPELINE;
					*cmdArgv = '\0';
					cmdLine++;
					while(*cmdLine == ' ' || *cmdLine == '\t')
						cmdLine++;
					*rwFile = cmdLine;		// Next part of the command
					terminate = 1;
					break;
			}
			cmdLine++;
		}
		while((*cmdLine == ' ' || *cmdLine == '\t' || *cmdLine == '\n') && terminate == 0)
		{
			*cmdLine = '\0';
			cmdLine++;
		}
		cmdArgv++;			// In the above lines special tasks like Input
	}					// Redirection, etc. are marked and individual
	*cmdArgv = '\0';			// arguments are stored in array 'cmdArgv'
	return cmdArgc;
}




/****************      Function to remove extra spaces       *****************
 ****************      from filename
 ****************  
 ***************/  


void trim(char *cmdLine)
{
	while(*cmdLine != ' ' && *cmdLine != '\t' && *cmdLine != '\n')
	{
		cmdLine++;
	}
	*cmdLine = '\0';
}




/****************      Function to execute the command        *****************
 ****************
 **************** This function accepts command line parameters in
 **************** the first parameter, mode of execution in the second
 **************** parameter and the file name as the third parameter
 **************** 
 ***************/  


void execute(char **cmdArgv, int mode, char **rwFile)
{
	pid_t pid, pid2;
	FILE *fp;
	int mode2 = SIMPLE, cmdArgc, status1, status2;
	char *cmdArgv2[CMD_LENGTH], *pipeFile = NULL;
	int myPipe[2];
	if(mode == PIPELINE)
	{
		if(pipe(myPipe))				// create pipe
		{
			fprintf(stderr, "Pipe failed!");
			exit(-1);
		}
		parse(*rwFile, cmdArgv2, &pipeFile, &mode2);	// processing next part of the command
	}							// after pipe
	pid = fork();					// creating child process to run the command
	if( pid < 0)
	{
		printf("Error occured");
		exit(-1);
	}
	else if(pid == 0)
		{
		switch(mode)
			{
			case REDIRECT_OUT:
				fp = fopen(*rwFile, "w+");	// open new file to write output 
				dup2(fileno(fp), 1);
				break;
			case APPEND_OUT:
				fp = fopen(*rwFile, "a");	// open file to append output
				dup2(fileno(fp), 1);
				break;
			case REDIRECT_IN:
				fp = fopen(*rwFile, "r");	// open file to read input
				dup2(fileno(fp), 0);
				break;
			case PIPELINE:
				close(myPipe[0]);		// close input end of pipe
				dup2(myPipe[1], fileno(stdout));// copy file to stdout
				close(myPipe[1]);		// then close output end of pipe
				break;
			}
		execvp(*cmdArgv, cmdArgv);			// this is run for plain commands
		}
	else
		{
		if(mode == BACKGROUND)
			{
			printf("[1] %d\n",pid);
			;
			}
		else if(mode == PIPELINE)
		{
			waitpid(pid, &status1, 0);		// wait for process 1 to finish
			pid2 = fork();				// creating process 2 to run
			if(pid2 < 0)				// the command that follows pipe
			{
				printf("error in forking");
				exit(-1);
			}
			else if(pid2 == 0)
			{
				close(myPipe[1]);		// close output end of pipe
				dup2(myPipe[0], fileno(stdin));	// copy file to stdin
				close(myPipe[0]);		// then close input end of pipe
				execvp(*cmdArgv2, cmdArgv2);	//    this is run for second command
			}					//    in a pipe
			else
			{
				;//wait(NULL);
				close(myPipe[0]);		// close input end of pipe
				close(myPipe[1]);		// close output end of pipe
			}
		}
		else
			waitpid(pid, &status1, 0);		// wait for process 1 to finish
			
	}
}



/****************      Function to display signal disable message        *****************
 ****************
 **************** This function is called when program break signals
 **************** are received and displays the message
 ***************/  


void handle_signal(int signo)
{
	printf("\n[Ctrl-C|Ctrl-d|Ctrl-z ignored\n ");
	fflush(stdout);
}



/****************      Function to expand wild chars like '*' and '?'    *****************
 ****************
 **************** The first param of the func takes the commandline string and 
 **************** looks for wildcard characters.  If any such char is 
 **************** found, then filename entries are read from the directory
 **************** structure and matching files are pushed into string 'extCommand'
 ***************/ 


int myGlob(char *expr, char **extCommand)
{
	DIR *dp;
	struct dirent *dirp;
	char *dirname=".";				// only current directory is searched for files
	char *filename;
	char *str;
	char *temp;
	int found=1,j=0;
	char s[2]=" ";
	int nomatch=1;

	str=malloc(sizeof(char *)*50);
	str=strtok(expr,s );					// commandline passed to this func is split into
								// separate words based on space (' ') then each
 	while(str!=NULL)					// word is examined to see whether it contains '?' or '*'
	{
		if(index(str,'?')!=NULL)			// if the '?' is found it is processed in the following lines
		{
			if ((dp=opendir("."))==NULL)		// opening the current directory structure
			{
				printf("cant open ");
				exit(1);
			}

			while ((dirp=readdir(dp))!=NULL)	// reading each file entry
			{
				found=1;
				temp=str;			// word from the command is stored in 'temp'
				filename=dirp->d_name;		// filename from directory entry is stored in 'filename'

				while(*temp!='\0' )
				{
					if (*temp !='?' && *temp!=*filename)
					{
						found=0;	// means name not matching
						break;
					}

					if(*temp == '?')	// if '?' appears, that char is skipped in both
					{			// the word and filename
						*temp++;
						*filename++;
					}

					*temp++;		// moving to next chars
					*filename++;
				}			
				if (found == 1)
				{
					nomatch=0;
					strcat(*extCommand,dirp->d_name);	// if a name match is found it is pushed into
					strcat(*extCommand," ");		// the string 'extCommand' and a space is added
				}
			}

			closedir(dp);
		}

		else

		if(index(str,'*')!=NULL)			// if the '*' is found it is processed in the following lines
		{
			if ((dp=opendir("."))==NULL)
			{
				printf("cant open ");
				exit(1);
			}
			while ((dirp=readdir(dp))!=NULL)
			{
				temp=str;
				filename=dirp->d_name;
				found=0;
				while(*temp!='\0' )		// upto here same as above where '?' is processed
				{
					if (*temp !='*' && *temp!=*filename)	// a non-star char is not matching. So 
						break;				// skip processing the filename
					while (*temp==*filename    )
					{
						*temp++;			// scan the chars in word and filename
						*filename++;			// as long as they are matching
					}
					if(*temp=='*' && *(temp+1) == '\0')	// if '*' is found as last char in word
					{					// it means match is found. So set 
						found=1;			// 'found' flag to 1 (true) and skip
						break;
					}
					if(*temp == '*')			// When a '*' is found in the word, look
					{					// for next char in word and search for
						*temp++;			// that char in filename
						while(*temp!=*filename    )
						{
							if(*temp=='\0' || *filename=='\0')
							break;			// if end of word or filename reached, stop
							*filename++;		// search further
						}
					while (*temp==*filename    && *temp!='\0')
					{					// continue matching until end of word is reached
						*temp++;
						*filename++;
					}
					if(*temp == *filename )
						found=1;
					else
						found=0;
					}
					*temp++;
					*filename++;
				}			

				if (found == 1)
				{
					nomatch=0;
					strcat(*extCommand,dirp->d_name);	// if a name match is found it is pushed into
					strcat(*extCommand," ");		// the string 'extCommand' and a space is added
				}
			}

			closedir(dp);
		}

		else
		{
			strcat(*extCommand,str);			// other non-* and non-? words of the command line
			strcat(*extCommand," ");			// are pushed into the string 'extCommand and a space is added.
		}
		str=strtok(NULL,s);
	}
	if (nomatch == 1)
		{
		strcat(*extCommand,temp);
		strcat(*extCommand," ");
		}
	return ;
}



