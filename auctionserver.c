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
#define TEMP_FILE_2 "temp2.txt"
#define TEMP_FILE_1 "temp1.txt"
#define HOST "127.0.0.1"
#define PHASE1_TCP_PORT "1100"
#define PHASE2_TCP_PORT "1200"
#define PHASE3_UDP_BID1 "3100"
#define PHASE3_UDP_BID2 "3200"
#define MAXBUFLEN 100
/*Are suppose to be dynamic*/
#define PHASE3_TCP "7000"
#define PHASE3_UDP_AS_BID1_PORT "6200"
#define PHASE3_UDP_AS_BID2_PORT "6300"


struct bid {
    char name[100];
    char item[100];
    char price[100];
};


void print_bidarray(struct bid *b, int size){
    printf("----------\n"); 
    int i; for(i=0;i<size;i++)
        printf("%s\t%s\t%s\n", b[i].name, b[i].item, b[i].price);
    printf("----------\n"); 
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

int phase3(){
    struct addrinfo hints, *bid_addr, *myaddr;
    char buf[1000], mesg[1000];
    struct sockaddr their_addr; 
    socklen_t addr_len;

    memset(&hints, 0, 1000);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int i;
    for(i=1;i<=2;i++){
        char port[100], tempfile[100], myport[100];
        strcpy(port, "");
        if(i==1){
            strcpy(port,PHASE3_UDP_BID1);
            strcpy(tempfile,TEMP_FILE_1);
            strcpy(myport, PHASE3_UDP_AS_BID1_PORT);
        } 
        else{
            strcpy(port, PHASE3_UDP_BID2);
            strcpy(tempfile,TEMP_FILE_2);
            strcpy(myport, PHASE3_UDP_AS_BID2_PORT);
        }

        if((getaddrinfo(HOST, myport, &hints, &myaddr)) != 0){
            perror("GetAddrInfo");
            exit(0);
        }

        /*PRINT*/
        printf("Phase 3: Auction Server IP Address: %s Auction UDP Port Number: %s.\n", HOST, myport);

        int sock = socket(myaddr->ai_family, myaddr->ai_socktype, 0);
        if(sock==-1){
            perror("Socket");
            exit(0);
        }

        if(bind(sock, myaddr->ai_addr, myaddr->ai_addrlen)==-1){
            perror("Bind");
            exit(0);
        }

        if((getaddrinfo(HOST, port, &hints, &bid_addr)) != 0){
            perror("GetAddrInfo");
            exit(0);
        }

        read_BFILE(mesg);
        /*Send broadcast list to the client*/
        if((sendto(sock, mesg, MAXBUFLEN-1, 0,  bid_addr->ai_addr, bid_addr->ai_addrlen)) == -1) {
            perror("sendto");
            exit(1);
        }

        /*Send broadcast information*/
        /*PRINT*/
        printf("Phase 3: Items for sale\n%s\n", mesg);

        int numbytes;
        if ((numbytes = recvfrom(sock, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        /*PRINT*/
        printf("Phase 3: Auction Server received a bidding from <Bidder%d>\n", i);
        /*Print bidding information*/
        /*PRINT*/
        printf("Phase 3: Bids received are\n%s", buf);

        /*Write biddings to temp file*/
        FILE *ft;
        ft = fopen(tempfile, "w");
        fprintf(ft, "%s", buf);
        fclose(ft);
    }
}

int phase3_decision(){
    int size1, size2, size;
    struct bid *b1  = (struct bid*)(malloc(sizeof(struct bid)*6));
    struct bid *b2  = (struct bid*)(malloc(sizeof(struct bid)*6));
    struct bid *bcast  = (struct bid*)(malloc(sizeof(struct bid)*6));

    file_to_bid_struct(b1, TEMP_FILE_1, &size1);
    file_to_bid_struct(b2, TEMP_FILE_2, &size1);
    file_to_bid_struct(bcast, BFILE, &size1);

    compare(bcast, size, b1, size1, b2, size2);

    /*Make TCP Connection and send result*/

    /*PRINT*/
    printf("End of Phase 3 for Auction Server.\n");
    return 0;
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

    if((getaddrinfo(HOST, PHASE2_TCP_PORT, &hints, &res)) != 0){
        perror("GetAddrInfo");
        exit(0);
    }


    /*PRINT*/
    printf("Phase 2: Auction Server IP Address: %s PreAuction TCP Port Number: %s\n", HOST, PHASE2_TCP_PORT);

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

    if((getaddrinfo(HOST, PHASE1_TCP_PORT, &hints, &res)) != 0){
        perror("GetAddrInfo");
        exit(0);
    }
    /*PRINT*/
    printf("Phase 1: Auction server has TCP port number %s and IP address %s\n", PHASE1_TCP_PORT, HOST);

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
    phase3();
    phase3_decision();
}
