#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<signal.h>

// int findlencmd(char* cmd){
// 	int i = 0;
// 	int spaces = 0;
// 	while(cmd[i]!= '\0'){
// 		if(cmd[i] == ' ') spaces++;
//         i++;
// 	}
// 	return spaces + 1;
// }

int executeCommand (char *cmd) {
	// pid_t pid;
	// pid = fork();
	// if(pid<0){
	// 	perror("fork");
    //     exit(-1);
	// }
	// Breaking the cmd into main command and its options by splitting it by " ".
	char* buf = strtok(cmd," ");
	char* excmd [20];
	int i = 0;
	while(buf!=NULL){
		excmd[i] = buf;
        printf("%s\n",excmd[i]);
        i++;
		buf = strtok(NULL," ");
	}
    excmd[i] = NULL;
    return 0;
    
    // for(int j = 0; j <i ;j++){
    //     printf("%s",excmd[i]);
    // }
	// if(!pid){ //Child process
	// 	char* exec_path = getenv("CS330_PATH");
	// 	char* token = strtok(exec_path, ":"); 
	// 	char* path = NULL;
	//     while (token != NULL) { 
    // 	    //process token here
	// 		char* f = strcat(token,excmd[0]);
	// 		if(open(f,O_RDONLY)!=-1){
	// 			path = f;
	// 			execv(path,excmd);
	// 		}
	// 		// find new token again 
    //     	token = strtok(NULL, ":"); 
    // 	}
		 
	// }
}

int main () {
    int in = open("inputs/input2.txt",O_RDONLY);
	close(0); // close stdin
	dup2(in,0); // dup to stdin for fgets()
	char buf[32];
	while(fgets(buf,31,stdin)){
		// printf("inside the loop\n");
		int len = strlen(buf);
		buf[len-1] = '\0';
		printf("%s %ld\n",buf,strlen(buf));

	}
	// printf("I was here too\n");
    return 0;
}

