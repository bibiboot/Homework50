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
#define PHASE3_TCP_SELLER1_PORT "2100"
#define PHASE3_TCP_SELLER2_PORT "2200"
#define PHASE3_TCP_BIDDER1_PORT "4100"
#define PHASE3_TCP_BIDDER2_PORT "4200"
#define MAXBUFLEN 100
/*Are suppose to be dynamic*/
#define PHASE3_TCP "7000"
#define PHASE3_UDP_AS_BID1_PORT "6200"
#define PHASE3_UDP_AS_BID2_PORT "6300"

char SELLER1[100] = "Taylor";
char SELLER2[100] = "Julia";

struct bid {
    char name[100];
    char item[100];
    char price[100];
    int bidtype;
};


void print_bidarray(struct bid *b, int size){
    /*Print the structure b*/
    printf("----------\n"); 
    int i; 
    for(i=0;i<size;i++){
        printf("%s\t%s\t%s\n", b[i].name, b[i].item, b[i].price);
    }
    printf("----------\n"); 
}

int price_of(struct bid *b, int size, char *name, char *item){
    /*Traverses the array of bids looking for the name and item*/
    //printf("Looking for %s---and Looking for %s\n", name, item);
    int i;
    for(i=0;i<size;i++){
        //printf("Traversing %s---and Traversing for %s\n", b[i].name, b[i].item);
        if(strcmp(b[i].name, name)==0 && strcmp(b[i].item, item)==0){
            /*Match found, now return*/
            return atoi(b[i].price);
        }
    }
    return -1;
}

int compare(struct bid *broadCast, int size, struct bid *b1, int size1, struct bid *b2, int size2, struct bid *result, int *rsize){
    /*Compare the values of the bids and find the appropriate match*/ 
 
    int sell_price;
    int i, bidtype  ;
    (*rsize)=0;
    for(i=0;i<size;i++){
        int price1 = price_of(b1, size1, broadCast[i].name, broadCast[i].item);
        int price2 = price_of(b2, size2, broadCast[i].name, broadCast[i].item);
        if(price1 >= atoi(broadCast[i].price) && price1 >= price2){
            /*Bidder1 won*/
           printf("SOLD1: bid = %d bid2 = %d and orignal = %s\n", price1, price2, broadCast[i].price);
           sell_price = price1;
           (*rsize)++; 
           bidtype = 1;
        }
        else if(price2 >= atoi(broadCast[i].price) && price2 >= price1){
            /*Bidder2 won*/
           sell_price = price2;
           printf("SOLD2: bid = %d bid2 = %d and orignal = %s\n", price1, price2, broadCast[i].price);
           (*rsize)++; 
           bidtype = 2;
        }
        else{
            //printf("NOT SOLD: bid = %d bid2 = %d and orignal = %s\n", price1, price2, broadCast[i].price);
            continue;
        }

        /*Fill the result structure*/
        char st[15];
        sprintf(st, "%d", sell_price);
        strcpy(result[i].name, broadCast[i].name);
        strcpy(result[i].item, broadCast[i].item);
        strcpy(result[i].price, st);
        result[i].bidtype = bidtype;
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
    /*Return 0 is*/ 
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
        printf("Phase 3: Items for sale\n%s", mesg);

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

int phase3_send_decision(int size, struct bid *result, char *client_name, struct addrinfo *their_addr, int sock  ){
    /*Seller uses this to send message*/
    char result_mesg[50], total_mesg[500];
    strcpy(total_mesg, "");
    if(connect(sock, their_addr->ai_addr, their_addr->ai_addrlen)<0){
         perror("Conect");
         exit(1);
    }
    int i;
    for(i=0; i<size; i++){
        if(strcmp(result[i].name, client_name  )==0){
            sprintf(result_mesg, "Phase 3: Item %s was sold at price %s\n", result[i].item, result[i].price);
            strcat(total_mesg, result_mesg);
       }
        else{
            //sprintf(result_mesg, "Phase 3: Item %s was not sold at price %s\n", result[i].item, result[i].price);
            //printf("%s\t %s",result[i].name, result_mesg);
            continue;
        }
    }
    printf("%s", total_mesg);
    if(send(sock, total_mesg, 200, 0) < 0){
        perror("Send\n");
        exit(1);
    }
}

int phase3_send_decision_bidder(int size, struct bid *result, struct addrinfo *their_addr, int sock, int bidtype  ){
    /*Bidder uses this to send message*/
    char result_mesg[50], total_mesg[500];
    strcpy(total_mesg, "");
    if(connect(sock, their_addr->ai_addr, their_addr->ai_addrlen)<0){
         perror("Conect");
         exit(1);
    }
    int i;
    for(i=0; i<size; i++){
        if(result[i].bidtype == bidtype){
            sprintf(result_mesg, "Phase 3: Item %s was sold at price %s\n", result[i].item, result[i].price);
            strcat(total_mesg, result_mesg);
       }
        else{
            //sprintf(result_mesg, "Phase 3: Item %s was not sold at price %s\n", result[i].item, result[i].price);
            //printf("%s\t %s",result[i].name, result_mesg);
            continue;
        }
    }
    printf("%s", total_mesg);
    if(send(sock, total_mesg, 200, 0) < 0){
        perror("Send\n");
        exit(1);
    }
}

int phase3_decision(){
    int size1, size2, size;
    struct bid *b1  = (struct bid*)(malloc(sizeof(struct bid)*6));
    struct bid *b2  = (struct bid*)(malloc(sizeof(struct bid)*6));
    struct bid *bcast  = (struct bid*)(malloc(sizeof(struct bid)*6));
    struct bid *result  = (struct bid*)(malloc(sizeof(struct bid)*6));

    file_to_bid_struct(b1, TEMP_FILE_1, &size1);
    file_to_bid_struct(b2, TEMP_FILE_2, &size2);
    file_to_bid_struct(bcast, BFILE, &size);

    int rsize;
    compare(bcast, size, b1, size1, b2, size2, result, &rsize);

    /*Make TCP Connection and send result*/

    int i;
    struct addrinfo hints, *bid1, *bid2, *sel1, *sel2, *myaddr;
    char mesg[100];
    strcpy(mesg, "Hello\n");

    memset(&hints, 0, 100);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(HOST, PHASE3_TCP_SELLER1_PORT, &hints, &sel1);
    getaddrinfo(HOST, PHASE3_TCP_SELLER2_PORT, &hints, &sel2);
    getaddrinfo(HOST, PHASE3_TCP_BIDDER1_PORT, &hints, &bid1);
    getaddrinfo(HOST, PHASE3_TCP_BIDDER2_PORT, &hints, &bid2);
    getaddrinfo(HOST, "8020", &hints, &myaddr);

    //print_bidarray(result, rsize);

    int sock = socket(bid1->ai_family, bid1->ai_socktype, 0);
    if(bind(sock, myaddr->ai_addr, myaddr->ai_addrlen)==-1){
        perror("Bind");
        exit(0);
    }

    phase3_send_decision(rsize, result, SELLER1, sel1, sock);

    int sock2 = socket(bid1->ai_family, bid1->ai_socktype, 0);
    if(bind(sock2, myaddr->ai_addr, myaddr->ai_addrlen)==-1){
        perror("Bind");
    }
    phase3_send_decision(size, result, SELLER2, sel2, sock2);

    int sock3 = socket(bid1->ai_family, bid1->ai_socktype, 0);

    if(bind(sock3, myaddr->ai_addr, myaddr->ai_addrlen)==-1){
        perror("Bind");
    }
    phase3_send_decision_bidder(size, result, bid1, sock3, 1);
    close(sock3);

    int sock4 = socket(bid1->ai_family, bid1->ai_socktype, 0);
    if(bind(sock4, myaddr->ai_addr, myaddr->ai_addrlen)==-1){
        perror("Bind");
    }
    phase3_send_decision_bidder(size, result, bid2, sock4, 2);

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

   fp = fopen(BFILE, "w+");
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
        printf("Phase 2: Writing\n%s", mesg);

        /*After two sellers are added*/
        if(count++==1) break;
    }
    fclose(fp);

    return 0;
}


int phase1(){
    /*Check the authentication of the seller and bidders*/
    /*Add the seller name to the lost*/
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
        if(count++==3)break;

    }
    printf("End of Phase 1 for Auction Server\n");
    return 0;
}

int main(int argc, char *argv[]){
    /*Main function*/
    //phase1();
    phase2();
    phase3();
    phase3_decision();
    return 0;
}
