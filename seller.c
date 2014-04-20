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
#define PHASE1_PORT "1100"
#define PHASE2_PORT "1200"
#define PHASE3_PORT_SELLER1 "2100"
#define PHASE3_PORT_SELLER2 "2200"
#define HOST "127.0.0.1"

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
    /*Reads through the cred file and creates
    auth request string
    */
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
    int i=0;
    FILE *fp;
    char buff[100], name[100], path[100], mesg[100], tmesg[1000];
    sprintf(path, "itemList%d.txt", btype);
    strcpy(tmesg, "");

    /*Open the itemsList*/
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

    /*Send auth request to auction server*/
    if(send(sock, tmesg, 100, 0) == -1){
        perror("Send\n");
    }

    /*PRINT*/
    printf("Phase 2: <Seller%d> send item lists.\n", btype);
    printf("Phase 2: \n%s", tmesg);
    return 0;
}

int phase3(int btype, char *port){
    /*
    PHASE3
    Open new tcp connection with the AS.
    Send the itemList to the AS.
    Close the server
    */
    char buff[500];
    struct addrinfo hints, *res;
    struct sockaddr cli_addr;
    int client_addr_size = sizeof(cli_addr);

    memset(&hints, 0, 100);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(HOST, port, &hints, &res);

    int sock = socket(res->ai_family, res->ai_socktype, 0);
    if(sock==-1){
        perror("Socket");
        exit(0);
    }

    if(bind(sock, res->ai_addr, res->ai_addrlen)==-1){
        perror("Bind");
        exit(0);
    }

    if(listen(sock, 10)==-1){
        perror("Listen");
        exit(0);
     }

    printf(".....Waiting..........%d\n", btype);
    int new_sock = accept(sock, &cli_addr, &client_addr_size);
    if(new_sock==-1){
        perror("Accept");
    }
    printf(".....Connected........\n");

    int num;
    if((num = recv(new_sock, buff, 500, 0)) == -1){
        perror("Recv\n");
        exit(1);
    }
    /*PRINT*/
    printf("%s", buff);

    close(new_sock);
    close(sock);

    /*PRINT*/
    printf("Phase 3: End of Phase 3 for <Seller%d>.\n", btype);
}


int phase2(int btype){
    /*
    PHASE2
    Open new tcp connection with the AS.
    Send the itemList to the AS.
    Close the server
    */
    struct addrinfo hints, *res;

    memset(&hints, 0, 50);
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

    /*PRINT*/
    printf("Phase 2: Auction Server IP Address: %s PreAuction Port Number: %s.\n", HOST, PHASE2_PORT);

    /*Read from itemList and send to the auction server*/
    read_itemList(btype, client_sock);

    /*PRINT*/
    printf("End of Phase 2 for <Seller%d>.\n", btype);

    /*Closing the socket*/
    close(client_sock);
    return 0;
}


void phase1(int btype){
    /*Authentication of the bidder takes place here*/

    char *client_ip;
    int client_port;
    char mesg[100], buff[100];
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

    close(client_sock);

}

int seller1(){
    /*Bidder two run by parent process*/
    phase1(1);
    sleep(10);
    phase2(1);
    phase3(1, PHASE3_PORT_SELLER1);
    return 0;
}

int seller2(){
    /*Bidder two run by child process*/
    phase1(2);
    //sleep(10);
    phase2(2);
    phase3(2, PHASE3_PORT_SELLER2);
    return 0;
}

int main(int argc, char *argv[]){
    /*Flow starts here*/
    int pid;

    /*Fork for two sellers*/
    if((pid=fork())==0){
        seller2();
    }
    else{
        seller1();
    }
    
    /*Wait for the child to over itself*/
    int status;
    waitpid(pid, &status, 0);
    return 0;
}
