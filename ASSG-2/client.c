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

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(serverip);
    
    while(1){
        char buffer[1024];
        fgets(buffer,sizeof(buffer),stdin);
        buffer[strcspn(buffer,"\n")]='\0';

        char ans[2048];
        snprintf(ans,sizeof(ans),"%s#%s",rollno,buffer);

        sendto(sockfd, ans, strlen(ans)+1, 0, (struct sockaddr *)&server_addr,sizeof(server_addr));

        if(strcmp(buffer,"EXIT")==0){
            break;
        }

        char msg[1024];
        socklen_t len = sizeof(server_addr);
        recvfrom(sockfd, msg, sizeof(msg), 0,(struct sockaddr *)&server_addr, &len);

        printf("Response from Server %s: %s\n",serverip,msg);
    }

    close(sockfd);
    return 0;
}