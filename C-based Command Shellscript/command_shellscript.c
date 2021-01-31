
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define TOKEN_DELIMITERS " \t\n\r\a" // DELIMITERS - To ignore different tags when parser tokenizes the command

static char * permittedCommands[] = {"pwd","nano","cat","wc","cp","sort","grep"};
static char * wc_unpermittedFlags[] = {"-k","-m"}; // Permitted flags: -l,-w,-c
static char * sort_unpermittedFlags[] = {"-b","-d", "-f","-g","-i","-M","-h","-n","-R","-V"}; // Permitted flags: -r
static char * grep_unpermittedFlags[] = {"-V","-E","-F","-G","-P","-e","-f","-i","-v","-w","-x","-y","-L","-l","-m","-o","-q","-s","-b","-H","-h","-n","-T","-u","-Z","-A","-B","-C","-a"}; // Permitted flags: -c
static char * cat_unpermittedFlags[] = {"-A","-b","-e","-E","-n","-s","-t","-v","-T"}; // No flags permitted
static char * cp_unpermittedFlags[] = {"-a","-f","-i","-l","-L","-n","-R","-u","-v"}; // No flags permitted

//Manual info execution
int execMan(char ** parserArgs) {
	int status,pid;
	if (fork() == 0) {
		if (execvp(parserArgs[0], parserArgs) == -1) printf("Error in execution");
		exit(0);
	}
	else pid = wait(&status);
	return 1;
}

// Change directory function
int changeDir(char * path) {
	if (path == NULL) printf ("No directory mentioned. Please enter the directory path after <cd> command\n");
	else if (chdir(path) != 0) printf("Current workspace has no such directory or path: %s\n", path);
	return 1;
}

// Function implements 'cat >" command to open new file and start writing into it
void catWrite(char * fileName) {
	int pid, status;

	if (fork() == 0) {
		int fd1 = creat(fileName,0644);
		int fd2 = dup2(fd1,1);
		char * args[] = {"cat",NULL};
		close(fd1);
		execv("/bin/cat", args);
		close(fd2);
		exit(0);
	}
	else pid = wait(&status);
}

// Manual information function
int man(char ** parserArgs) {
	if (parserArgs[1] == NULL) {
		printf("Shell-script implemented in C\n-------------------------------\n");
		printf("Supported commands: <cd> <wc> <sort> <grep> <pwd> <cat> <cp>\n");
		printf("Pipeline supported commands: <wc> <sort> <grep> <cat> (as output) <pwd>\n");
		printf("This script doesn't supports multiple pipelines.\n");
		printf("Beware that pipeline can work only with certain commands (just as in UNIX system itself");
		printf("To exit the script simply enter the command <exit> with no arguments\n");
		printf("To find more information on commands enter <man [command]>\n");
	}
	else {
		if (strcmp(parserArgs[1],"wc") == 0) printf("NOTE! THIS SCRIPT SUPPORTS ONLY <-l>, <-c>, <-w> FLAGS FOR THIS COMMAND\n");
		else if (strcmp(parserArgs[1],"sort") == 0) printf("NOTE! THIS SCRIPT SUPPORTS ONLY <-r> FLAG FOR THIS COMMAND\n");
		else if (strcmp(parserArgs[1],"grep") == 0) printf("NOTE! THIS SCRIPT SUPPORTS ONLY <-c> FLAG FOR THIS COMMAND\n");
		else if (strcmp(parserArgs[1],"cat") == 0) printf("NOTE! THIS SCRIPT SUPPORTS NO FLAGS FOR THIS COMMAND\n");
		else if (strcmp(parserArgs[1],"cp") == 0) printf("NOTE! THIS SCRIPT SUPPORTS NO FLAGS FOR THIS COMMAND\n");
		else if (strcmp(parserArgs[1],"pwd") != 0 && strcmp(parserArgs[1],"nano") != 0) {
			printf("This script doesn't supports manual for <%s> command\n",parserArgs[1]);
			return 1;
		}
		return execMan(parserArgs);
	}
	return 1;
}

// Command correctness (command + [options/flags]) validation function (according to the task)
int validateCommand(char ** command, int isPipe) {
	int validCommand = 0; // Checking if the command permitted (initial assumption: no)
	int validFlag = 1; // Checking if the flags (options) are permitted (initial assumption: yes)
	int validArgsExist = 1; // Checking if the minimal number of arguments is right (initial assumption: yes)

	for (int i = 0; i < (sizeof(permittedCommands) / sizeof(char *)); i++) {
		if (strcmp(permittedCommands[i],command[0]) == 0) {
			validCommand = 1;
			if (strcmp("wc",command[0]) == 0) {
				if (command[1] != NULL) {
					for (int j = 0; j < (sizeof(wc_unpermittedFlags) / sizeof(char *)); j++) {
						if (strcmp(command[1],wc_unpermittedFlags[j]) == 0) {
							validFlag = 0;
							j = (sizeof(wc_unpermittedFlags) / sizeof(char *));
						}
					}
					if (((strcmp(command[1],"-l") == 0) || (strcmp(command[1],"-w") == 0) || (strcmp(command[1],"-c") == 0)) && command[2] == NULL) validArgsExist = isPipe;
				}
				else validArgsExist = isPipe; 
			}
			else if (strcmp("sort",command[0]) == 0) {
				if (command[1] != NULL) {
					for (int j = 0; j < (sizeof(sort_unpermittedFlags) / sizeof(char *)); j++) {
						if (strcmp(command[1],sort_unpermittedFlags[j]) == 0) {
							validFlag = 0;
							j = (sizeof(wc_unpermittedFlags) / sizeof(char *));
						}
					}
					if (strcmp(command[1],"-r") == 0 && command[2] == NULL)  validArgsExist = isPipe;
				}
				else validArgsExist = isPipe;
			}
			else if (strcmp("grep",command[0]) == 0 && command[1] != NULL) {
				for (int j = 0; j < (sizeof(grep_unpermittedFlags) / sizeof(char *)); j++) {
					if (strcmp(command[1],grep_unpermittedFlags[j]) == 0) {
						validFlag = 0;
						j = (sizeof(grep_unpermittedFlags) / sizeof(char *));
					}
				}
			}
			else if (strcmp("cat",command[0]) == 0) {
				if (command[1] != NULL) {
					for (int j = 0; j < (sizeof(cat_unpermittedFlags) / sizeof(char *)); j++) {
						if (strcmp(command[1],cat_unpermittedFlags[j]) == 0) {
							validFlag = 0;
							j = (sizeof(cat_unpermittedFlags) / sizeof(char *));
						}
					}
				}
				else validArgsExist = 0;
			}
			else if (strcmp("cp",command[0]) == 0) {
				if (command[1] != NULL && command[2] != NULL) {
					for (int j = 0; j < (sizeof(cp_unpermittedFlags) / sizeof(char *)); j++) {
						if (strcmp(command[1],cp_unpermittedFlags[j]) == 0) {
							validFlag = 0;
							j = (sizeof(cp_unpermittedFlags) / sizeof(char *));
						}
					}
				}
				else validArgsExist = 0;
			}
			i = (sizeof(permittedCommands) / sizeof(char *));
		}
	}
	if (!validCommand) {
		printf("Command <%s> not supported by this shell-script.\n", command[0]);
		return 0;
	}
	if (!validArgsExist) {
		printf("Command <%s> has no enough arguments to procceed.\n", command[0]);
		return 0;
	}
	if (!validFlag) {
		printf("This script doesn't supports <%s> flag for <%s> command.\n",command[1],command[0]);
		return 0;
	}
	return 1;
}

// Parsed command line execution function (for single command without pipeline)
int execCommand(char ** parserArgs) {
	int status,pid;
	if (validateCommand(parserArgs,0) != 0) {
		if (fork() == 0) {
			if (execvp(parserArgs[0], parserArgs) == -1) printf("Error in execution");
			exit(0);
		}
		else pid = wait(&status);
	}
	return 1;
}

// Function parses command line into tokens (for execution)
char ** commandParser(char * commandLine) {
	int pos = 0;
	char * token;
	char ** tokens = malloc(sizeof(char*) * 64);
	token = strtok(commandLine, TOKEN_DELIMITERS);
	
	while (token != NULL) {
		tokens[pos] = token;
		pos++;
		token = strtok(NULL, TOKEN_DELIMITERS);
	}

	tokens[pos] = NULL;
	return tokens;
}

//Function executing pipeline (due to complexity of specific task instructions some checks will be implemented within this function) 
int execPipe(char * commandLine) {
	char * second;
	char * first = strtok_r(commandLine,"|",&second);
	char ** firstCommand = commandParser(first);
	char ** secondCommand = commandParser(second);
	pid_t pid;
	int fd[2];
	int status;
	
	if ((strcmp(firstCommand[0],"cp") == 0) || (strcmp(firstCommand[0],"nano") == 0) || (strcmp(firstCommand[0],"man") == 0) || (strcmp(firstCommand[0],"cd") == 0) || (strcmp(firstCommand[0],"exit") == 0)) {
		printf("Command <%s> is not supported by pipeline output in this script\n",firstCommand[0]);
		return 1;
	}
	else if ((strcmp(secondCommand[0],"cp") == 0) || (strcmp(secondCommand[0],"nano") == 0) || (strcmp(secondCommand[0],"man") == 0) || (strcmp(secondCommand[0],"cd") == 0) || (strcmp(secondCommand[0],"exit") == 0) || (strcmp(secondCommand[0],"cat") == 0)) {
		printf("Command <%s> is not supported by pipeline input in this script\n",secondCommand[0]);
		return 1;
	}
	else if ((strcmp(firstCommand[0],"cat") == 0) && (strcmp(firstCommand[1],">") == 0)){
		printf("Command 'cat >' is not supported by pipeline in this script");
		return 1;
	}
	else if (validateCommand(firstCommand,0) && validateCommand(secondCommand,1)) {
		pipe(fd);
		pid = fork();

		if (pid == 0) {
			dup2(fd[1],1);
			close(fd[0]);
			close(fd[1]);
			execvp(firstCommand[0],firstCommand);
			exit(1);
		}
		else {
			pid = fork();
			if (pid == 0) {
				dup2(fd[0],0);
				close(fd[1]);
				close(fd[0]);
				
				execvp(secondCommand[0],secondCommand);
				exit(1);
			}
			else {
				close(fd[0]);
				close(fd[1]);
				pid = wait(&status);
			}
			pid = wait(&status);
		}
	}
	return 1;
}

//Function checks if there is a pipeline ("|"): Error if more than one pipeline, continues accordingly if one or no pipeline
int checkPipeline (char * commandLine) {
	int count = 0; // Counting the number of pipelines ("|")
	int i = 0;

	while (i < strlen(commandLine)) {
		if (commandLine[i] == '|') count++;
		i++;
	}

	if (count > 1) printf("This shell-script doesn't supports multiple pipelines!\n");
	return count;
}

// Function validates the type of the given command (<exit> and <cd> and "cat >" require different implementation)
int validateType(char ** parserArgs) {
	if (strcmp(parserArgs[0],"exit") == 0) return 0;
	else if (strcmp(parserArgs[0],"cd") == 0) {
		if (parserArgs[2] != NULL) printf ("<cd> command requires only one argument as a path, further arguments ignored\n");
		return changeDir(parserArgs[1]);
	}
	else if (strcmp(parserArgs[0],"cat") == 0 && parserArgs[1] != NULL && strcmp(parserArgs[1],">") == 0 && parserArgs[2] != NULL) {
		catWrite(parserArgs[2]);
		return 1;
	}
	else if (strcmp(parserArgs[0],"man") == 0) return man(parserArgs);
	else return execCommand(parserArgs);
}

// Function reads the command line written by the user
char * readCommandLine() {
	// Pre-set
	int pos = 0;
	int commandBufferSize = 1024;
	int inputChar;
	char * commandBuffer = malloc(sizeof(char) * commandBufferSize);

	while(1) {
		inputChar = getchar(); // Reading the command by each character

		if (inputChar == '\n' || inputChar == EOF) {
		// End of command
			commandBuffer[pos] = '\0';
			return commandBuffer;
		}
		else commandBuffer[pos] = inputChar;
		
		pos++;
	}
}

// Main
int main (int argc, int **argv) {
	char * commandLine; // Command line
	char ** parserArgs; // Arguments resulted from command line
	int result = 1; // Used to continue/break the loop
	int ctype;

	printf("Welcome to Vit's shell-script (implemented in C)!\n---------------------------------------------------\n");
	while(result) {
		printf("\nEnter the command: "); // Text for user
		commandLine = readCommandLine(); // Reading of user command
		if (strlen(commandLine) > 0) {
			ctype = checkPipeline(commandLine); //Checking for pipelines				
			if (ctype == 0) {
				parserArgs = commandParser(commandLine); // Parsing the command into arguments
				result = validateType(parserArgs); // Starting the chain of validation + execution
			}
			else if (ctype == 1) result = execPipe(commandLine);
			else result = 1;
		}
	}

	free(commandLine);
	free(parserArgs);
	printf("Goodbye!\n");
	return 0;
}
