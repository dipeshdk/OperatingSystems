#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main(){
    for(int i = 0 ; i < 5 ; i++){ 
        pid_t pid = fork();
        int fd[2];
        if(pid > 0){
            close(1);
            dup2(fd[1],1);
            waitpid(pid,&status,0);
        }else{
            close(0);
            dup2(fd[0],0);
            execl("./a.out","./a.out",NULL);
            exit(0);
        }
    }
    return 0;
}
