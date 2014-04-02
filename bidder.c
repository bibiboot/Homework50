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

#define CRED_FILE1 "bidderPass1.txt"
#define CRED_FILE2 "bidderPass2.txt"

char PORT[100] = "9001";

void get_cred(char *result){
    FILE *fp = fopen(CRED_FILE1, "r");
    char line[100];
    char ty[100], uname[100], pwd[100], account[100];
    while(fgets(line, 100, fp)!=NULL){
        sscanf(line, "%[^ ] %[^ ] %[^ ] %s", ty, uname, pwd, account);
    }
    //printf("%s\t%s\t%s\t%s", ty, uname, pwd, account);
    sprintf((result), "Login#%s %s %s", uname, pwd, account);
    printf("Valus = %s\n", result);
}

void start(){

    struct addrinfo hints, *res;
    char buff[9000];
    char mesg[100];


    memset(&hints, 0, 100);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo("127.0.0.1", PORT, &hints, &res);

    int client_sock = socket(res->ai_family, res->ai_socktype, 0);
    connect(client_sock, res->ai_addr, res->ai_addrlen);

    get_cred(mesg);
    strcat(mesg, "X");

    printf("BIDDER: Send %s\n", mesg);
    if(send(client_sock, mesg, 100, 0) == -1){
        perror("Send\n");
    }

    if(recv(client_sock, buff, 9000, 0) == -1){
        perror("Recv\n");
    }
    printf("BIDDER: Recieved %s\n", buff);
}

int bidder1(){
    /*Bidder two run by parent process*/
    //printf("Parent\n");
    start();

    return 0;
}

int bidder2(){
    /*Bidder two run by child process*/
    //printf("Child\n");
    start();
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
        //bidder1();

    }
    
    /*Wait for the child to over itself*/
    int status;
    waitpid(pid, &status, 0);
}

