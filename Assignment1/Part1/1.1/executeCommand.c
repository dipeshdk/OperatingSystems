#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>

int executeCommand (char *cmd) {
	pid_t pid = fork();
	if(!pid){
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
			strcpy(path,token);         // path
			strcat(path,"/");           // path/
			strcat(path,excmd[0]);      // path/program_name
			strcat(path,"\0");          // path/program_name\0    <---> '\0' to end the string
			execv(path,excmd); 
			token = strtok(NULL,":\n\0");
		}

		// If the execv fails then only the program will reach this line and thus exit with value -1
		printf("UNABLE TO EXECUTE\n");
		exit(-1);
	}
	return 0;
}

int main (int argc, char *argv[]) {
	return executeCommand(argv[1]);
}
