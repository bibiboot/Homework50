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
#define PHASE3_UDP_BID1_PORT "3100"
#define PHASE3_UDP_BID2_PORT "3200"
#define PHASE3_UDP_AS_BID1_PORT "6200"
#define PHASE3_UDP_AS_BID2_PORT "6300"

#define MAXBUFLEN 100

char PORT[100] = "1100";
char HOST[100] = "127.0.0.1";

int read_FILE(char *tmesg, char *path){
    /*Read from the correct itemList.txt and
    */
    FILE *fp;
    char buff[100];
    strcpy(tmesg, "");
    fp = fopen(path, "r");

    while(fgets(buff, 100, fp)!=NULL){
        /*Rest of the lines create message and send*/
        strcat(tmesg, buff);
    }
    return 0;
}

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

int phase3(int btype, char *port, char *servport){
    struct addrinfo hints, *server, *myaddr;
    char mesg[1000];
    //struct sockaddr_storage their_addr;
    struct sockaddr their_addr;
    char buf[1000];
    socklen_t addr_len;

    memset(&hints, 0, 1000);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if((getaddrinfo(HOST, servport, &hints, &server)) != 0){
        perror("GetAddrInfo");
        exit(0);
    }

    if((getaddrinfo(HOST, port, &hints, &myaddr)) != 0){
        perror("GetAddrInfo");
        exit(0);
    }

    /*PRINT*/
    printf("Phase 3: <Bidder%d#> has UDP port %s and IP address: %s\n", btype, port, HOST);

    int sock = socket(myaddr->ai_family, myaddr->ai_socktype, 0);
    if(sock==-1){
        perror("Socket");
        exit(0);
    }

    if(bind(sock, myaddr->ai_addr, myaddr->ai_addrlen)==-1){
        perror("Bind");
        exit(0);
    }

   int numbytes;
   if ((numbytes = recvfrom(sock, buf, MAXBUFLEN-1 , 0, &their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    /*PRINT*/
    printf("Phase 3: Items up for sale\n%s\n", buf);

    sleep(10);
    read_FILE(mesg, "bidding1.txt");
    /*Send bids to the AS*/
    if((sendto(sock, mesg, MAXBUFLEN-1, 0, server->ai_addr, server->ai_addrlen)) < 0) {
        perror("sendto");
        exit(1);
    }

    /*PRINT*/
    printf("Phase 3: <Bidder#%d> Make bidding for\n%s", btype, mesg);

    /*PRINT*/
    printf("End of Phase 3 for Bidder%d.\n", btype);
}


void phase1(int btype){

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
    //phase1(1);
    phase3(1, PHASE3_UDP_BID1_PORT, PHASE3_UDP_AS_BID1_PORT);
    return 0;
}

int bidder2(){
    /*Bidder two run by child process*/
    //phase1(2);
    phase3(2, PHASE3_UDP_BID2_PORT, PHASE3_UDP_AS_BID2_PORT);
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
