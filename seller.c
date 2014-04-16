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

#define CRED_FILE1 "sellerPass1.txt"
#define CRED_FILE2 "sellerPass2.txt"

char PHASE1_PORT[100] = "9011";
char PHASE2_PORT[100] = "9002";
char HOST[100] = "127.0.0.1";

int ipaddr_client(struct sockaddr cli_addr, char **human_cli_addr){
    /*Convert sockaddr to human string ip address*/
    struct sockaddr_in *cli_addr_in = (struct sockaddr_in*)(&cli_addr);
    *human_cli_addr = inet_ntoa(cli_addr_in->sin_addr);
    return 0;
}

int strip_newline(char *given, char *result){
    /*Removes the newline from the given string*/
    /*Result is stored in the result string*/
    while(*given!='\0'){
        if(*given=='\n'){
            given++;
            continue;
        }
        *result=*given;
        given++;
        result++;
    }
    *result='\0';
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


int read_itemList(int btype, int sock){
    /*Read from the correct itemList.txt and
      send it to the authentication server
    */
    FILE *fp;
    int i=0;
    char buff[100], name[100], path[100], mesg[100], tmesg[1000];
    sprintf(path, "itemList%d.txt", btype);
    strcpy(tmesg, "");

    fp = fopen(path, "r");

    while(fgets(buff, 100, fp)!=NULL){
        if(i==0){
            /*First line is username*/
            strip_newline(buff, name);
            i++;
            continue;
        }
        i++;
        /*Rest of the lines create message and send*/
        sprintf(mesg, "%s %s", name, buff);
        strcat(tmesg, mesg);
    }
    if(send(sock, tmesg, 100, 0) == -1){
        perror("Send\n");
    }
    /*PRINT*/
    printf("Phase 2: <Seller%d> send item lists.\n", btype);
    printf("Phase 2: %s", tmesg);
    return 0;
}

int phase2(int btype){
    /*
    PHASE2
    Open new tcp connection with the AS.
    Send the itemList to the AS.
    Close the server
    */
    int client_port;
    char mesg[100], buff[100];
    char *client_ip;
    struct addrinfo hints, *res;

    memset(&hints, 0, 100);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(HOST, PHASE2_PORT, &hints, &res);

    int client_sock = socket(res->ai_family, res->ai_socktype, 0);
    if(client_sock==-1){
        perror("Socket");
        exit(0);
    }
    if(connect(client_sock, res->ai_addr, res->ai_addrlen)<0){
        perror("Connect");
        exit(0);
    }

    ipaddr_client(*(res->ai_addr), &client_ip);
    struct sockaddr_in *cli_addr_in = (struct sockaddr_in*)(res->ai_addr);

    /*PRINT*/
    printf("Phase 2: Auction Server IP Address: %s PreAuction Port Number: %s.\n", HOST, PHASE2_PORT);

    /*Read from itemList and send to the auction server*/
    read_itemList(btype, client_sock);

    /*PRINT*/
    printf("End of Phase 2 for <Seller%d>.\n", btype);
    return 0;
}


void phase1(int btype){

    int client_port;
    char mesg[100], buff[100];
    char *client_ip;
    struct addrinfo hints, *res;

    memset(&hints, 0, 100);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(HOST, PHASE1_PORT, &hints, &res);

    int client_sock = socket(res->ai_family, res->ai_socktype, 0);
    if(client_sock==-1){
        perror("Socket");
        exit(0);
    }
    if(connect(client_sock, res->ai_addr, res->ai_addrlen)<0){
        perror("Connect");
        exit(0);
    }

    ipaddr_client(*(res->ai_addr), &client_ip);
    struct sockaddr_in *cli_addr_in = (struct sockaddr_in*)(res->ai_addr);

    /*PRINT*/
    printf("Phase 1: <Seller%d> has TCP port %d and IP address: %s \n", btype, cli_addr_in->sin_port, client_ip);

    get_cred(btype, mesg);
    /*strcat(mesg, "X");*/

    if(send(client_sock, mesg, 100, 0) == -1){
        perror("Send\n");
    }

    if(recv(client_sock, buff, 9000, 0) == -1){
        perror("Recv\n");
    }
    /*PRINT*/
    printf("Phase 1: Login request reply: %s.\n", buff);
    printf("End of Phase 1 for <Seller%d>.\n", btype);

}

int seller1(){
    /*Bidder two run by parent process*/
    phase1(1);
    sleep(10);
    phase2(1);
    return 0;
}

int seller2(){
    /*Bidder two run by child process*/
    phase1(2);
    sleep(10);
    phase2(2);
    return 0;
}

int main(int argc, char *argv[]){
    int pid;

    if((pid=fork())==0){
        seller2();
    }
    else{
        seller1();
    }
    
    /*Wait for the child to over itself*/
    int status;
    waitpid(pid, &status, 0);
}
