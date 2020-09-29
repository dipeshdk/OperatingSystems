#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h> 
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include "gameUtils.h"

#define ROCK        0 
#define PAPER       1 
#define SCISSORS    2 

#define STDIN 		0
#define STDOUT 		1
#define STDERR		2

int getWalkOver(int numPlayers); // Returns a number between [1, numPlayers]


// This fucntion is for reading the input char by char using read and
// then stop reading as it encounters \n.
// Thus, this is a function similar to fgets().
void get_input(int info, char* str){
	char buf[1];
	int i =0;
	while(read(info,buf,1)){
		if(buf[0]!= '\n'){
			str[i] = buf[0];
			i++;
		}else{
			break;
		}
	}
	str[i] = '\0';
}


// This function is to implement the strtok() function kind of effect to
// take out the program name from the path for giving the program name to execv.
char* get_options(char* player_binary, char* program_name){
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

int rounds = 10;

int main(int argc, char *argv[])
{
	// updating the value of round if given
	if(argc > 2){
		rounds = atoi(argv[2]);
	}

	// opening the file for reading
	int info = open(argv[argc-1],O_RDONLY);
	
	//getting the number of players from the file players.txt
	char players_count[101]; 
	get_input(info,players_count);
	int n = atoi(players_count);
	
	// getting the binaries of the players in the tournament
	char player_binaries[n][101];
	for(int i =0 ; i<n ;i++){
		get_input(info,player_binaries[i]);
	}
	close(info);

	// setting up communication channels betweeen the parent and the child process
	int pipe1[n][2]; // for sending the input to child
	int pipe2[n][2]; // for getting the output from the child
	
	for(int i=0; i<n; i++){
		// building pipes
		if(pipe(pipe1[i]) < 0) exit(-1);
		if(pipe(pipe2[i]) < 0) exit(-1);
		
		pid_t pid = fork();
		if(pid < 0) exit(-1);
		
		// getting the program name for sending it to the execv function
		char program_name[101];
		get_options(player_binaries[i],program_name);
		char* options[] = {program_name,NULL};
		
		if(!pid){
			// child
			dup2(pipe1[i][0],STDIN);
			close(pipe1[i][1]);
			dup2(pipe2[i][1],STDOUT);
			close(pipe2[i][0]);
			
			execv(player_binaries[i],options);
			// If execv() fails then this line will be implemented and thus the program will exit with value -1
			exit(-1);
		}

		// parent
		close(pipe1[i][0]);
		close(pipe2[i][1]);
	}

	//**********************Implementation of the tournament starts from here**********************************************

	int walkover_player_id = -1;
	int count_players = n;

	// Initialisation of the arrays
	int player_in_tournament[n]; // This array will tell whether a player still exists in the tournament or not.
	int score[n]; // This array will keep scores of all the players.
	for(int i =0 ; i<n ;i++){
		player_in_tournament[i] = 1; // players exists as it is the starting.
		score[i] = 0; // score = 0 as it is the starting.
	}
	

	while(count_players > 1){
		// odd players check and updating the walkover value
		if(count_players%2 == 1){
			walkover_player_id = getWalkOver(count_players)-1;
		}else{
			walkover_player_id = -1;
		}
		
		// for printing the players in the tournament
		int space = 1;
		for(int j=0; j<n;j++){
			if(player_in_tournament[j]){
				if(space < count_players)
					printf("p%d ",j);
				else printf("p%d",j);
				space++;
			}
		}
		printf("\n");

		int prev_index = -1;
		int flag = 0;
		
		for(int i = 0 ; i< rounds; i++){
			char move[n][1];
			// Implementing the round i for each pair of player
			for(int j = 0; j < n; j++){
				if(player_in_tournament[j] && j != walkover_player_id){
					write(pipe1[j][1], "GO\0", 3);
					read(pipe2[j][0], move[j], 1);
					
					if(flag){
						if(move[j][0] != move[prev_index][0]){
							if( ((move[j][0] == '1') && (move[prev_index][0] == '0')) || ((move[j][0] == '0') && (move[prev_index][0] == '2')) || ((move[j][0] == '2') && (move[prev_index][0] == '1'))) score[j]++;
							else score[prev_index]++;
						}
						flag = 0;
						prev_index = -1;
					}else{
						prev_index = j;
						flag = 1;
					}
				}
			}
		}

		// updating the players who won and thus still exist in the tournament
		prev_index = -1;
		flag = 0;
		for(int j =0; j < n; j++){
			if(player_in_tournament[j] && j != walkover_player_id){
				if(flag){
					if(score[j] > score[prev_index]){
						player_in_tournament[prev_index] = 0;
						score[j] = 0;
					}else {
						player_in_tournament[j] = 0;
						score[prev_index] = 0;
					}
					flag = 0;
				}else{
					prev_index = j;
					flag = 1;
				}
			}
		}

		
		count_players = (count_players+1)/2;
		
		if(count_players == 1){
			for(int j=0; j<n;j++){
				if(player_in_tournament[j]){
					printf("p%d",j);
					break;
				}
			}
		}
	}
	
	return 0;
}
