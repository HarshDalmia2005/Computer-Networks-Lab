#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PORT 30031

int main(int argc, char *argv[]){
    if(argc!=3){
        printf("%s",argv[1]);
        printf("Server IP nad Roll Required\n");
        exit(EXIT_FAILURE);
    }

    char *serverip=argv[1];
    char *rollno=argv[2];

    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(serverip);

    printf("Client started...");
    connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
    printf("Client Connected...");
    
    while(1){
        char buffer[1024];
        fgets(buffer,sizeof(buffer),stdin);
        buffer[strcspn(buffer,"\n")]='\0';

        char ans[2048];
        snprintf(ans,sizeof(ans),"%s#%s",rollno,buffer);

        write(sockfd, ans, strlen(ans)+1);

        if(strcmp(buffer,"EXIT")==0 || strcmp(buffer,"CONTINUE")==0){
            break;
        }

        char msg[1024];
        socklen_t len = sizeof(server_addr);
        read(sockfd, msg, sizeof(msg));

        printf("Response from Server %s: %s\n",serverip,msg);
    }

    close(sockfd);
    return 0;
}