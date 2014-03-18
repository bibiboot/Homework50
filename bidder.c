#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/wait.h> 

int bidder1(){
    /*Bidder two run by parent process*/
    printf("Parent\n");

    return 0;
}

int bidder2(){
    /*Bidder two run by child process*/
    printf("Child\n");

    return 1;
}



int main(int argc, char *argv[]){
    int pid;
    if((pid=fork())==0){
        /*Child*/
        bidder2();
    }
    else{
        /*Parent*/
        bidder1();

    }
    
    /*Wait for the child to over itself*/
    int status;
    waitpid(pid, &status, 0);

}

