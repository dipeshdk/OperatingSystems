#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<signal.h>

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

int main(){
    char program_name[101];
    get_options("tests/test1/player1\n",program_name);
    printf("%s",program_name);
    return 0;
}