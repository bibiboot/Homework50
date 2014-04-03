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
char HOST[100] = "127.0.0.1";

int ipaddr_client(struct sockaddr cli_addr, char **human_cli_addr){
    /*Convert sockaddr to human string ip address*/
    struct sockaddr_in *cli_addr_in = (struct sockaddr_in*)(&cli_addr);
    *human_cli_addr = inet_ntoa(cli_addr_in->sin_addr);
    return 0;
}

int get_cred(int btype, char *result){
    FILE *fp;
    if(btype==1)
        fp = fopen(CRED_FILE1, "r");
    else
        fp = fopen(CRED_FILE2, "r");

    char ty[100], uname[100], pwd[100], account[100], line[100];
    while(fgets(line, 100, fp)!=NULL){
        sscanf(line, "%[^ ] %[^ ] %[^ ] %s", ty, uname, pwd, account);
    }
    sprintf((result), "Login#%s %s %s", uname, pwd, account);
    /*PRINT*/
    printf("Phase 1: Login request. User: %s password: %s Bank account: %s\n", uname, pwd, account);
    return 0;
}

void start(int btype){

    struct addrinfo hints, *res;
    char buff[9000];
    char mesg[100];

    memset(&hints, 0, 100);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo("127.0.0.1", PORT, &hints, &res);

    char *client_ip;
    int client_sock = socket(res->ai_family, res->ai_socktype, 0);
    connect(client_sock, res->ai_addr, res->ai_addrlen);

    ipaddr_client(*(res->ai_addr), &client_ip);
    struct sockaddr_in *cli_addr_in = (struct sockaddr_in*)(res->ai_addr);

    get_cred(btype, mesg);
    //strcat(mesg, "X");

    /*PRINT*/
    printf("Phase 1: <Bidder#>%d has TCP port %d and IP address: %s \n", btype, cli_addr_in->sin_port, client_ip);
    if(send(client_sock, mesg, 100, 0) == -1){
        perror("Send\n");
    }

    if(recv(client_sock, buff, 9000, 0) == -1){
        perror("Recv\n");
    }
    /*PRINT*/
    printf("Phase 1: Login request reply: %s.\n", buff);
}

int bidder1(){
    /*Bidder two run by parent process*/
    //printf("Parent\n");
    start(1);

    return 0;
}

int bidder2(){
    /*Bidder two run by child process*/
    //printf("Child\n");
    start(2);
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

