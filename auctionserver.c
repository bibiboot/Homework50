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

#define ACCEPT "#Accepted"
#define REJECT "#Rejected"
#define PATH "Registration.txt"
#define BFILE "broadcastList.txt"
#define HOST "127.0.0.1"
#define PORT "9011"
#define PHASE2_PORT "9002"
#define PHASE3_PORT "9003"
#define PHASE3_UDP_BIDDER1_PORT "9004"
#define PHASE3_UDP_BIDDER2_PORT "9005"
#define MAXBUFLEN 100

struct bid {
    char name[100];
    char item[100];
    char price[100];
};


int ipaddr_client(struct sockaddr cli_addr, char **human_cli_addr){
    /*Convert sockaddr to human string ip address*/
    struct sockaddr_in *cli_addr_in = (struct sockaddr_in*)(&cli_addr);
    *human_cli_addr = inet_ntoa(cli_addr_in->sin_addr);
    return 0;
}

int read_BFILE(char *tmesg){
    /*Read from the correct itemList.txt and
    */
    FILE *fp;
    char buff[100];
    strcpy(tmesg, "");
    fp = fopen(BFILE, "r");

    while(fgets(buff, 100, fp)!=NULL){
        /*Rest of the lines create message and send*/
        strcat(tmesg, buff);
    }
    return 0;
}

int text_to_bid_struct(struct bid *b){
    FILE *fp;
    char buff[100];
    fp = fopen(BFILE, "r");
    int i=0;
    while(fgets(buff, 100, fp)!=NULL){
        sscanf(buff, "%[^ ] %[^ ] %s", b[i].name, b[i].item, b[i].price);
        i++;
    }
    return 0;
}

int file_to_bid_struct(struct bid *b, char *path, int *size){
    FILE *fp;
    char buff[100];
    *size = 0;
    fp = fopen(path, "r");
    while(fgets(buff, 100, fp)!=NULL){
        sscanf(buff, "%[^ ] %[^ ] %s", b[*size].name, b[*size].item, b[*size].price);
        (*size)++;
    }
    return 0;
}




int authenticate(char *mesg, char *client_ip){
    /*Return 0 is 
    /*Message format: "Login#username password bankaccount*/
    char username[100], password[100], bankaccount[100];
    char *arg = strchr(mesg, '#')+1;
    //printf("Arg: %s\n", arg);
    char *temp = strdup(arg);
    char *token;
    int i=0;

    while((token = strsep(&temp, " "))!=NULL){
        //printf("%s\n", token); 
        if(i==0)
            strcpy(username, token);
        else if(i==1)
            strcpy(password, token);
        else if(i==2)
            strcpy(bankaccount, token);
        else
            return -1;
        i++; 
    }

    //printf("Username = %s, Password = %s and BankAccount = %s\n", username, password, bankaccount);
    FILE *fp;
    fp = fopen(PATH, "r");
    char buff[100];
    char fileusername[100], filepassword[100], filebackaccount[100], auth_mesg[100];
    strcpy(auth_mesg, REJECT);

    while(fgets(buff, 500, fp)!=NULL){
        //printf("%s\n", buff);
        sscanf(buff, "%[^ ] %[^ ] %s", fileusername, filepassword, filebackaccount);
        if(strcmp(username, fileusername)==0 && strcmp(password, filepassword)==0 && strcmp(bankaccount, filebackaccount)==0){
            strcpy(auth_mesg, ACCEPT);
            break;
        }
    }
    /*PRINT*/
    printf("Phase 1: Authentication request. User#: Username %s Password: %s Bank Account: %s User IP Addr: %s. Authorized: %s\n", username, password, bankaccount, client_ip, auth_mesg);

    if(strcmp(ACCEPT, auth_mesg)==0)
        return 0;
    return -1;
}

int phase3(char *port){
    /*PRINT*/
    printf("Phase 3: Auction Server IP Address: %s Auction UDP Port Number: %s\n", HOST, PHASE3_PORT);

    struct addrinfo hints, *res;
    char buf[MAXBUFLEN], mesg[MAXBUFLEN];
    struct sockaddr their_addr; 
    socklen_t addr_len;

    memset(&hints, 0, 1000);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if((getaddrinfo(HOST, port, &hints, &res)) != 0){
        perror("GetAddrInfo");
        exit(0);
    }
    /*PRINT*/
    printf("Phase 3: Auction Server IP Address: %s Auction UDP Port Number: %s.\n", HOST, port);

    int sock = socket(res->ai_family, res->ai_socktype, 0);
    if(sock==-1){
        perror("Socket");
        exit(0);
    }


    read_BFILE(mesg);
    /*Send broadcast list to the client*/
    if((sendto(sock, mesg, MAXBUFLEN-1, 0, res->ai_addr, res->ai_addrlen)) == -1) {
        perror("sendto");
        exit(1);
    }


    int numbytes;
    if ((numbytes = recvfrom(sock, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
    printf("%s\n", buf);

    printf("Phase 3: (Item list displayed here)\n");

    printf("Phase 3: Auction Server received a bidding from <Bidder#>\n");

    printf("Phase 3: (Bidding information displayed here)\n");

    

    /*PRINT*/
    printf("End of Phase 3 for Auction Server.\n");
}


int phase2(){
    // Socket Connection
    FILE *fp;
    /*Open the broadcast file
      if not present, then create
    */
    fp = fopen(BFILE, "w");
    struct addrinfo hints, *res;
    char mesg[1000], auth_mesg[100];

    memset(&hints, 0, 1000);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if((getaddrinfo(HOST, PHASE2_PORT, &hints, &res)) != 0){
        perror("GetAddrInfo");
        exit(0);
    }
    /*PRINT*/
    printf("Phase 2: Auction Server IP Address: %s PreAuction TCP Port Number: %s\n", HOST, PHASE2_PORT);

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

    int count=0;

    while(1){
        printf(".....Waiting..........\n");
        struct sockaddr cli_addr;
        char *client_ip;

        int client_addr_size = sizeof(cli_addr);
        int new_sock = accept(sock, &cli_addr, &client_addr_size);
        if(new_sock==-1){
            perror("Accept");
        }
        printf(".....Connected........\n");
        /*Get client address*/
        ipaddr_client(cli_addr, &client_ip);

        /*Recieve*/
        if(recv(new_sock, mesg, 1000, 0)<0){
            perror("Recv");
        }

        /*Write to broadcastList.txt*/
        fprintf(fp, "%s", mesg);

        /*PRINT*/
        printf("Phase 2: %s", mesg);

        /*After two sellers are added*/
        if(count++==1) break;
    }
    printf("End of Phase 2 for Auction Server\n");
    return 0;
}




int phase1(){
    // Socket Connection
    struct addrinfo hints, *res;
    char mesg[1000], auth_mesg[100];

    memset(&hints, 0, 1000);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if((getaddrinfo(HOST, PORT, &hints, &res)) != 0){
        perror("GetAddrInfo");
        exit(0);
    }
    /*PRINT*/
    printf("Phase 1: Auction server has TCP port number %s and IP address %s\n", PORT, HOST);

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

    int count = 0;

    while(1){
        printf(".....Waiting..........\n");
        struct sockaddr cli_addr;
        char *client_ip;

        int client_addr_size = sizeof(cli_addr);
        int new_sock = accept(sock, &cli_addr, &client_addr_size);
        if(new_sock==-1){
            perror("Accept");
        }
        printf(".....Connected........\n");
        /*Get client address*/
        ipaddr_client(cli_addr, &client_ip);

        /*Recieve*/
        if(recv(new_sock, mesg, 1000, 0)<0){
            perror("Recv");
        }

        /*Check auhentication*/
        if(authenticate(mesg, client_ip)==-1)
             strcpy(auth_mesg, "Rejected#");
        else
             strcpy(auth_mesg, "Accepted#");

        /*Send authentication message*/
        if(send(new_sock, auth_mesg, 100, 0) == -1){
            perror("Send\n");
        }

        /*In case of true authenticate
          Save the ip address and name together,
          Send the user the ipaddr and port num for
          next two phases.*/
        if(count++==1)break;

    }
    printf("End of Phase 1 for Auction Server\n");
    return 0;
}

void print_bidarray(struct bid *b){
    int i;
    for(i=0;i<=5;i++)
        printf("%s\t%s\t%s\n", b[i].name, b[i].item, b[i].price);
}

int price_of(struct bid *b, int size, char *name, char *item){
    /*Traverses the array of bids looking for the name and item*/
    printf("Looking for %s---and Looking for %s\n", name, item);
    int i;
    for(i=0;i<size;i++){
        printf("Traversing %s---and Traversing for %s\n", b[i].name, b[i].item);
        if(strcmp(b[i].name, name)==0 && strcmp(b[i].item, item)==0){
            /*Match found, now return*/
            return atoi(b[i].price);
        }
    }
    return -1;
}

int compare(struct bid *broadCast, int size, struct bid *b1, int size1, struct bid *b2, int size2){
    /*Compare the values of the bids and find the appropriate match*/ 
    int i;
    for(i=0;i<size;i++){
        int price1 = price_of(b1, size1, broadCast[i].name, broadCast[i].item);
        int price2 = price_of(b2, size2, broadCast[i].name, broadCast[i].item);
        printf("Price1 = %d Price2 = %d\n", price1, price2);
    }
}

void test1(){
    struct bid *barray  = (struct bid*)(malloc(sizeof(struct bid)*6));
    text_to_bid_struct(barray);
    struct bid *b1  = (struct bid*)(malloc(sizeof(struct bid)*6));
    text_to_bid_struct(b1);
    struct bid *b2  = (struct bid*)(malloc(sizeof(struct bid)*6));
    text_to_bid_struct(b2);
    compare(barray, 6, b1, 6, b2, 6);
}

void test2(){
    int size, size1, size2;
    struct bid *barray  = (struct bid*)(malloc(sizeof(struct bid)*6));
    file_to_bid_struct(barray, BFILE, &size);
    struct bid *b1  = (struct bid*)(malloc(sizeof(struct bid)*6));
    file_to_bid_struct(b1, "bidding1.txt", &size1);
    struct bid *b2  = (struct bid*)(malloc(sizeof(struct bid)*6));
    file_to_bid_struct(b2, "bidding2.txt", &size2);
    compare(barray, size, b1, size1, b2, size2);
}

int main(int argc, char *argv[]){
    /*Main function*/
    //phase1();
    //phase2();
    //phase3(PHASE3_UDP_BIDDER2_PORT);
    //phase3(PHASE3_UDP_BIDDER1_PORT);
    test2();
}
