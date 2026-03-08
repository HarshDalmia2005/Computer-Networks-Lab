#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define PORT 30031
#define MY_ROLL "23CH10031"

float perform_op(int arr[], int n, char *op){
    if(strcmp(op,"SUM")==0){
        int sum=0;
        for(int i=0;i<n;i++){
            sum+=arr[i];
        }
        return (float)sum;
    }
    else if(strcmp(op,"AVG")==0){
        int sum=0;
        for(int i=0;i<n;i++){
            sum+=arr[i];
        }
        return (float)sum/n;
    }
    else if(strcmp(op,"MAX")==0){
        int max=arr[0];
        for(int i=1;i<n;i++){
            if(arr[i]>max){
                max=arr[i];
            }
        }
        return (float)max;
    }
    else if(strcmp(op,"MIN")==0){
        int min=arr[0];
        for(int i=1;i<n;i++){
            if(arr[i]<min){
                min=arr[i];
            }
        }
        return (float)min;
    }
    else{
        return -1;
    }
}

int main(){
    int sockfd;
    struct sockaddr_in server_addr, client_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if(sockfd<0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int res=bind(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr));

    if(res<0){
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server running on port %d\n", PORT);

    while(1){
        char buffer[1024];
        socklen_t len=sizeof(client_addr);
        int n=recvfrom(sockfd,buffer,sizeof(buffer)-1,0,(struct sockaddr *)&client_addr,&len);
        buffer[n]='\0';

        printf("Client: %s\n",buffer);

        char *roll=strtok(buffer,"#");
        char *msg=strtok(NULL,"#");

        if(strcmp(roll,MY_ROLL)!=0){
            char *terminal="ERROR: Unauthorized client";
            sendto(sockfd,terminal,strlen(terminal)+1,0,(struct sockaddr *)&client_addr,len);
            continue;
        }

        printf("Received message from %s: %s\n",inet_ntoa(client_addr.sin_addr),msg);
        if(strcmp(msg,"EXIT")==0){
            break;
        }
        
        char *op=strtok(msg,"|");
        int size=atoi(strtok(NULL,"|"));
        char *a=strtok(NULL,"|");

        int arr[size];
        arr[0]=atoi(strtok(a," "));
        for(int i=1;i<size;i++){
            arr[i]=atoi(strtok(NULL," "));
        }

        float res=perform_op(arr,size,op);
        char ans[1024];

        sprintf(ans,"%f",res);
        sendto(sockfd,ans,strlen(ans)+1,0,(struct sockaddr *)&client_addr,len);
    }

    close(sockfd);
    return 0;
}