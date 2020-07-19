#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define MAX_LINE 80

// Read command when user enters input
void readLineInShell(char* line)
{
	char temp;
	int pos = 0;

	if(!line)
	{
		fprintf(stderr, "Allocation error\n");
		exit(EXIT_FAILURE);
	}
	int should_run = 1;
	while (should_run)
	{
		temp = getchar();
		if(temp != '\n')
			line[pos++] = temp;
		else
		{
			line[pos] = '\0';
			should_run = 0;
			break;
		}

		if(pos > MAX_LINE + 1)
		{
			line = new char[pos];
			if(!line)
			{
				fprintf(stderr, "Allocation error\n");
				exit(EXIT_FAILURE);
			}	
		}
	}
}

//Split the string into multiple strings if you see the signs below
#define STRING_TOKEN_SPECIAL " \n\t><|"
char** normalSplitLine(char* line)
{
	if(line == NULL)
		return NULL;
	//count the space and tab
	int count = 0; 
	for (int i = 0; i < strlen(line); i++)
	{
		if(line[i] == ' ' || line[i] == '\t')
			count++;
	}
	//Dynamic allocation for count + 2 elements
	char** args = new char*[count + 2];
	
	//Split line by use function strtok
	char* token;
	int pos = 0;
	if(!args)
	{
		fprintf(stderr, "Allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, STRING_TOKEN_SPECIAL);
	while(token != NULL)
	{
		args[pos++] = token;
		token = strtok(NULL, STRING_TOKEN_SPECIAL);
	}
	args[pos] = NULL;
	return args;
}

void normalLauch(char* line)
{
	char** args;
	args = normalSplitLine(line);
	pid_t pid;
	int status; 
	if((pid = fork()) == -1)
		perror("osh");
	else if(pid == 0)
	{
		if(execvp(args[0], args) < 0)
			perror("osh");
		exit(EXIT_FAILURE);
	}
	else
	{
		do 
		{
      		waitpid(pid, &status, WUNTRACED);
  		} 
  		while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	delete[] args;
}

//Copy string a to string b
char* copyCommand(char* a)
{
	char* b = new char[(sizeof(a)/sizeof(char)) + 1];
	int pos = 0;
	while(a[pos] != '\0')
	{
		b[pos] = a[pos];
		pos++;
	}
	b[pos] = '\0';
	return b;
}

int classifyCommand(char* line)
{
	//ReadLauch
	if (strchr(line,'<') != NULL)
		return 1;
	//WriteLauch
	else if(strchr(line, '>') != NULL)
		return 2;
	//PieLauch
	else if(strchr(line, '|') != NULL)
		return 3;
	//history
	else if (strcmp(line, "!!") == 0)
		return 4;
	//normal
	else return 0;
}

void readLauch(char *line)
{
	char **args;
	args = normalSplitLine(line);
	pid_t pid;
	int status;
	if ((pid = fork()) == -1)
		perror("osh");
	else if (pid == 0)
	{
		int n = 0;
		while (args[n] != NULL)
		{
			n++;
		}

		FILE *file_in;
		file_in = fopen(args[n - 1], "r");
		args[n-1] = NULL;
		dup2(fileno(file_in), STDIN_FILENO);
		if (execvp(args[0], args) < 0)
			perror("osh");
		exit(EXIT_FAILURE);
	}
	else
	{
		do
		{
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	free(args);
}

void writeLauch(char *line)
{
	char **args;
	args = normalSplitLine(line);
	pid_t pid;
	int status;
	if ((pid = fork()) == -1)
		perror("osh");
	else if (pid == 0)
	{
		int n = 0;
		while (args[n] != NULL)
		{
			n++;
		}

		FILE *file_out;
		file_out = fopen(args[n - 1], "w");
		args[n-1] = NULL;
		dup2(fileno(file_out), STDOUT_FILENO);
		if (execvp(args[0], args) < 0)
			perror("osh");
		exit(EXIT_FAILURE);
	}
	else
	{
		do
		{
			waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	free(args);
}

void pipeLauch(char* line)
{
	char** args;
	args = normalSplitLine(line);
	
	int pos = 0, i = 0, j = 0, k = 0;
	// split args to args1 and args2
	
	while(args[i] != NULL) {
		if(*args[i] == '|') {
			pos = i;
		}
		i++;
	}

	char **argv1 = new char*[pos];
	char **argv2 = new char*[i - pos - 1];
	while(args[j] != NULL) {
		if(j < pos){
			argv1[j] = args[j];
			j++;
			continue;
		}
		if(j == pos){
			j++;
			continue;
		}
		else{
			argv2[k] = args[j];
			k++;
			j++;
			continue;
		}
	}

	
	

	int file_des[2];
	pid_t child1, child2;

	// make a pipe (fds go in pipefd[0] and pipefd[1])
	if(pipe(file_des) != 0) {
		printf( "\nCan't create Pipe");
		return;
	}
	child1 = fork();
	if(child1 < 0) {
		printf("\nCan't fork child 1");
		return;
	}

	// child1 != 0
	if(child1 == 0){
		// child1 executing...
		close(file_des[0]);
		dup2(file_des[1], STDOUT_FILENO);
		close(file_des[1]);

		if(execvp(argv1[0], argv1) < 0){
			printf("Can't execute command 1");
			exit(0);
		}
	}
	else {
		// Parent executing
		child2 = fork();
		
		if(child2 < 0){
			printf( "Can't fork child 2");
			return;
		}

		// child 2 executing
		if(child2 == 0) {
			close(file_des[1]);
			dup2(file_des[0], STDIN_FILENO);
			close(file_des[0]);
			if(execvp(argv2[0], argv2) < 0){
				printf( "Can't execute command 2");
				exit(0);
			}
		}
		else {
			wait(NULL);
			wait(NULL);
		}
	}


}

int main()
{
	char* line = new char[MAX_LINE + 1];
	int should_run = 1;
	char* history;
	while(should_run)
	{
		printf("osh> ");
		fflush(stdout);
		readLineInShell(line);
		
		//History support: use command "!!"
		if(strcmp(line, "!!") == 0)
		{
			if(!history)
				printf("No history command\n");
			else
			{
				delete[] line;
				line = copyCommand(history);
			}

		}
		//Exit: use command "exit"
		else if(strcmp(line, "exit") == 0)
		{
			should_run = 0;
			break;
		}
		else
		{
			if(history != NULL)
				delete[] history;
			history = copyCommand(line);
		}
		
		switch(classifyCommand(line))
		{
		case 0:
			normalLauch(line);
			break;
		
		case 1:
			readLauch(line);
			break;
		
		case 2:
			writeLauch(line);
			break;
		case 3:
			pipeLauch(line);
			break;
		case 4: 
			break;
		}
	}
	delete[] line;
	return 0;
}