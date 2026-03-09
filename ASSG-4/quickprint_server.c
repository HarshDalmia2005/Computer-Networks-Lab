#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>

/*
    precautions:
    1.Always leave room for null terminator
    2.never use an unintialised pointer
*/

typedef enum { QUEUED, PRINTING, DONE, CANCELED} job_state_t;

typedef struct{
    int id;
    char title[128];
    job_state_t state;
    struct timespec submitted_at;
    int canceled;
}job_t;

static double elapsed_seconds(struct timespec start, struct timespec now){
    double s=(double)(now.tv_sec-start.tv_sec);
    double ns=(double)(now.tv_nsec-start.tv_nsec)/1e9;
    return s+ns;
}

job_state_t compute_state(const job_t *j){
    if(j->canceled)return CANCELED;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC,&now);
    double elapsed=elapsed_seconds(j->submitted_at,now);
    
    if(elapsed<10.0)return QUEUED;
    if(elapsed<30.0)return PRINTING;
    return DONE;
}

char *state_to_string(job_state_t state){
    switch(state){
        case QUEUED:
            return "QUEUED";
        case PRINTING:
            return "PRINTING";
        case DONE:
            return "DONE";
        case CANCELED:
            return "CANCELED";
    }
    return "UNKNOWN";
}

job_t jobs[20];
int next_job_id=1;

int main(int argc, char *argv[]){

    if(argc!=2){
        printf("PORT NOT PROVIDED!\n");
        exit(1);
    }

    signal(SIGCHLD, SIG_IGN);

    int PORT=atoi(argv[1]);
    int sockfd,client_sock;
    
    struct sockaddr_in client_addr,server_addr;
    socklen_t len;
    char buffer[1024];
    
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0){
        perror("SOCKET");
        exit(1);
    }

    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    server_addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
        perror("BIND");
        exit(1);
    }

    if(listen(sockfd,5)<0){
        perror("LISTEN");
        exit(1);
    }

    printf("SERVER IS LISTENING ON PORT %d\n",PORT);

    while(1){
        socklen_t len = sizeof(client_addr);
        client_sock = accept(sockfd, (struct sockaddr*) &client_addr, &len);

        if(client_sock<0){
            continue;
        }

        printf("CLIENT CONNECTED FROM %s\n",inet_ntoa(client_addr.sin_addr));
        pid_t pid=fork();
        

        if(pid==0){
            close(sockfd);
            while(1){
                char msg[1024];
                int x=read(client_sock,msg,sizeof(msg)-1);

                if(x<=0){
                    close(client_sock);
                    exit(0);
                }

                msg[x]='\0';
                char *CMD=strtok(msg," ");

                if(strcmp(CMD,"HELLO")==0){
                    
                    char *username=strtok(NULL," ");
                    char msg[1024];
                    sprintf(msg,"WELCOME %s\n",username);
                    write(client_sock,msg,strlen(msg)+1);
                }
                else if(strcmp(CMD,"SUBMIT")==0){
                    char *title=strtok(NULL," ");
                    char data[4096];
                    char doc[4096]={0};
                    bool found=false;
                    int amount=0;
                    while((x=read(client_sock,data,sizeof(data)-1))>0 && !found){
                        data[x]='\0';

                        if(strcmp(data,".")==0){
                            found=true;
                            break;
                        }

                        if(amount+strlen(data)>=4096){
                            int space_left=4096-amount;
                            int copy_len=space_left-4;
                            if(copy_len>0){
                                snprintf(doc+amount,space_left,"%.*s...",copy_len,data);
                            }
                            break;
                        }
                        snprintf(doc+amount,4096-amount," %s",data);
                        amount+=strlen(data);
                    }

                    //now creating a jOB
                    jobs[next_job_id].id=next_job_id;
                    jobs[next_job_id].state=QUEUED;
                    strcpy(jobs[next_job_id].title,title);
                    clock_gettime(CLOCK_MONOTONIC,&jobs[next_job_id].submitted_at);
                    jobs[next_job_id].canceled=0;
                    next_job_id++;
                    

                    char msg[1024];  
                    sprintf(msg,"JOB ID: %d",next_job_id-1);
                    write(client_sock,msg,strlen(msg)+1);
                }
                else if(strcmp(CMD,"STATUS")==0){
                    char *jobid=strtok(NULL," ");
                    int id=atoi(jobid);
                    
                    if(id<0 || id>=next_job_id){
                        char msg[1024];
                        sprintf(msg,"JOB %d NOT FOUND\n",id);
                        write(client_sock,msg,strlen(msg)+1);
                        continue;
                    }

                    job_t *j=&jobs[id];
                    job_state_t state=compute_state(j);

                    char msg[1024];
                    sprintf(msg,"JOB %d: %s\n",id,state_to_string(state));
                    write(client_sock,msg,strlen(msg)+1);
                }else if(strcmp(CMD,"LIST")==0){
                    char response[8192]={0};
                    int total=next_job_id-1;
                    int offset=0;
                    offset+=sprintf(response+offset,"TOTAL JOBS: %d\n",total);
                    for(int i=1;i<next_job_id;i++){
                        job_state_t st=compute_state(&jobs[i]);
                        offset+=sprintf(response+offset,"  JOB %d: %-20s  %s\n",i,jobs[i].title,state_to_string(st));
                    }
                    write(client_sock,response,strlen(response)+1);
                }else if(strcmp(CMD,"CANCEL")==0){
                    char *jobid=strtok(NULL," ");
                    int id=atoi(jobid);

                    if(id<0 || id>=next_job_id){
                        char msg[1024];
                        sprintf(msg,"JOB %d NOT FOUND\n",id);
                        write(client_sock,msg,strlen(msg)+1);
                        continue;
                    }

                    job_t *j=&jobs[id];
                    job_state_t st=compute_state(j);

                    if(st!=DONE){
                        j->canceled=1;
                        char msg[1024];
                        sprintf(msg,"400: JOB %d CANCELED\n",id);
                        write(client_sock,msg,strlen(msg)+1);
                    }else{
                        char msg[1024];
                        sprintf(msg,"409: JOB %d ALREADY DONE\n",id);
                        write(client_sock,msg,strlen(msg)+1);
                    }
                }else if(strcmp(CMD,"QUIT")==0){
                    printf("CLIENT %s DISCONNECTED\n",inet_ntoa(client_addr.sin_addr));
                    char msg[1024];
                    sprintf(msg,"200: GOODBYE\n");
                    write(client_sock,msg,strlen(msg)+1);
                    close(client_sock);
                    exit(0);
                }else{
                    char msg[1024];
                    sprintf(msg,"400: BAD REQUEST\n");
                    write(client_sock,msg,strlen(msg)+1);
                }
            }
            printf("CLIENT %s DISCONNECTED\n",inet_ntoa(client_addr.sin_addr));
            close(client_sock);
            exit(0);
        }else {
            close(client_sock);
            continue;
        }
    }
    
    printf("SERVER SHUTTING DOWN...\n");
    close(sockfd);
    close(client_sock);
    return 0;
}