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
#define HOST "127.0.0.1"
#define PORT "7001"

/*
void load_registration(){

    FILE *fp;
    fp = fopen(PATH, "r");
    char buff[100];
    int i=0, j=0;

    while(fgets(buff, 500, fp)!=NULL){
        //printf("%s\n", buff);
        sscanf(buff, "%[^ ] %[^ ] %s", CRED[i][0], CRED[i][1], CRED[i][2]);
        i++;
    }

    for(i=0; i<=1; i++){
        for(j=0; j<=2; j++)
            printf("%s\t", CRED[i][j]);     
        printf("\n");
    }

}
*/


int authenticate(char *mesg){
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

    printf("Username = %s, Password = %s and BankAccount = %s\n", username, password, bankaccount);

    FILE *fp;
    fp = fopen(PATH, "r");
    char buff[100];
    char fileusername[100], filepassword[100], filebackaccount[100];

    while(fgets(buff, 500, fp)!=NULL){
        //printf("%s\n", buff);
        sscanf(buff, "%[^ ] %[^ ] %s", fileusername, filepassword, filebackaccount);
        if(strcmp(username, fileusername)==0 && strcmp(password, filepassword)==0 && strcmp(bankaccount, filebackaccount)==0){
            return 0;
        }
    }

    return -1;
}

void start(){
    // Socket Connection
    struct addrinfo hints, *res;
    char buff[1000];

    memset(&hints, 0, 1000);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if((getaddrinfo(HOST, PORT, &hints, &res)) != 0){
        perror("GetAddrInfo");
        exit(0);
    }

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

    while(1){
        printf(".....Waiting..........\n");
        int new_sock = accept(sock, res->ai_addr, &(res->ai_addrlen));
        if(new_sock==-1){
            perror("Accept");
        }
        printf(".........Connected..........\n");

        recv(new_sock, buff, 1000, 0);
    }

}


int main(int argc, char *argv[]){
    start();
    char *mesg = "Login#James pass123 451943546";
    if(authenticate(mesg)==-1){
        printf("Rejected\n");
    }
    else{
        printf("Accepted\n");
    }
}
