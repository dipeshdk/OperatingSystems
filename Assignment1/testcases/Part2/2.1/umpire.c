#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<signal.h>

// This function is to implement the strtok() function kind of effect to
// take out the program name from the path for giving the program name to execv.
void get_options(char* player_binary, char* program_name){
	int i = 0;
	int pos_slash = 0;
	while(player_binary[i]){
		if(player_binary[i] =='/'){
			pos_slash = i;
		}
        i++;
	}
	int k = 0;
	for(int j = pos_slash+1; j < i; j++, k++){
		program_name[k] = player_binary[j];
	}
	program_name[k] = '\0';
}


int main(int argc, char* argv[]) {
	// player 1
	int p11[2];
	int p12[2];

	if(pipe(p11) < 0) exit(-1); // pipe 1 for player 1
	if(pipe(p12) < 0) exit(-1); // pipe 2 for player 1
	
	pid_t pid = fork();
	if(pid < 0) exit(-1);
	
	if(!pid){
		dup2(p11[0],0); // read end of first pipe is stdin
		dup2(p12[1],1); // write end of second pipe is stdout
		
		// getting the program name for sending it to the execv function
		char program_name[101];
		get_options(argv[1],program_name);
		char* options[] = {program_name,NULL};
		execv(argv[1],options);
		
		// If execv() fails then this line will be implemented and thus the program will exit with value -1
		exit(-1);
	}


	//player 2
	int p21[2];
	int p22[2];

	if(pipe(p21) < 0) exit(-1); // pipe 1 for player 2
	if(pipe(p22) < 0) exit(-1);	// pipe 2 for player 2
	
	pid = fork();
	if(pid < 0) exit(-1);
	
	if(!pid){
		dup2(p21[0],0); // read end of first pipe is stdin
		dup2(p22[1],1); // write end of second pipe is stdout
		
		// getting the program name for sending it to the execv function
		char program_name[101];
		get_options(argv[2],program_name);
		char* options[] = {program_name,NULL};
		execv(argv[2],options);
		
		// If execv() fails then this line will be implemented and thus the program will exit with value -1
		exit(-1);
	}
	

	// umpire starts
	int score[] = {0,0};
	
	for(int i =0; i < 10; i++){
		// write GO
		write(p11[1],"GO\0",3);
		write(p21[1],"GO\0",3);
		
		char p1[1]; char p2[1];
		
		// Read Move
		read(p12[0],p1,1);
		read(p22[0],p2,1);

		//update scores
		if(p1[0] != p2[0]){
			if( ((p1[0] == ('1')) && (p2[0] == '0')) || ((p1[0] == '0') && (p2[0] == '2')) || ((p1[0] == '2') && (p2[0] == '1'))) score[0]++;
			else score[1]++;
		}
	}
	
	// Print Result
	printf("%d %d",score[0],score[1]);
	
	return 0;
}
