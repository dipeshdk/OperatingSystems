#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<signal.h>

// This command has been directly picked from executeCommand.c ie from 1.1
// Just the fork command is deleted from this command
int executeCommand (char *cmd) {

	// Breaking the cmd into main command and its options by splitting it by " ", "\n", "\0".
	char* excmd[11];
	char* buf = strtok(cmd," \n\0");
	int n = 0;
	while(buf!=NULL){
		excmd[n] = buf;
		buf = strtok(NULL," \n\0");
		n++;
	}
	excmd[n] = NULL;

	// get the path from the environment
	char* exec_path = getenv("CS330_PATH");
	
	// taking out the path from exec_path 
	char* token = strtok(exec_path, ":\n\0"); 
	while(token!=NULL){
		char path[strlen(token)];
		strcpy(path,token);				// path
		strcat(path,"/");				// path/
		strcat(path,excmd[0]);			// path/program_name
		strcat(path,"\0");				// path/program_name\0    <---> '\0' to end the string
		execv(path,excmd); 				// execute the command
		token = strtok(NULL,":\n\0");
	}
	
	// If the execv fails then only the program will reach this line and thus exit with value -1
	printf("UNABLE TO EXECUTE\n");
	exit(-1);
	return 0;
}


int execute_in_parallel(char *infile, char *outfile)
{
	// opening the input file here
	int in = open(infile,O_RDONLY);
	close(0); // close stdin
	dup2(in,0); // dup to stdin for fgets()

	// reading commands from infile line by line
	char buf[1000];
	char cmd[51][1000];
	int i = 0;
	while(fgets(buf,1000,stdin)!= NULL){
		strcpy(cmd[i],buf);
		int len = strlen(cmd[i]);
		cmd[i][len-1] = '\0';
		i++;
	}

	// opening the output file here
	int out = open(outfile, O_WRONLY | O_CREAT, 0644);
	close(1); // close stdout of parent
	dup2(out,1); // now everything will be printed to the file

	int pfd[i][2]; // the array of pipes , one for each children
	for(int j =0 ; j < i; j++){
		pipe(pfd[j]);		
		pid_t pid = fork();
		if(!pid){
			dup2(pfd[j][1],1);
			executeCommand(cmd[j]);
		}
	}

	// reading the output step wise from each of the children
	char buf1[1000];
	for(int j= 0; j < i; j++){
		int x = read(pfd[j][0],buf1,1000);
		if(x < 1000){
			buf1[x] = '\0';
		}
		printf("%s",buf1);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	return execute_in_parallel(argv[1], argv[2]);
}
