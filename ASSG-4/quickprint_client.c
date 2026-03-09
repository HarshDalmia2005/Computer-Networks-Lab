#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <strings.h>

void sigint_handler(int signum){
    printf("\nCLIENT DISCONNECTED...\n");
    exit(0);
}

void print_help(){
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║              QUICKPRINT PROTOCOL REFERENCE                      ║\n");
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║                                                                  ║\n");
    printf("║  AVAILABLE COMMANDS:                                             ║\n");
    printf("║  ─────────────────────────────────────────────────────────────── ║\n");
    printf("║                                                                  ║\n");
    printf("║  1. HELLO <username>                                             ║\n");
    printf("║     Greet the server and register your username.                 ║\n");
    printf("║     Request:   HELLO Alice                                       ║\n");
    printf("║     Response:  WELCOME Alice                                     ║\n");
    printf("║                                                                  ║\n");
    printf("║  2. SUBMIT <title>                                               ║\n");
    printf("║     Submit a document for printing.                              ║\n");
    printf("║     After typing the command, enter document content             ║\n");
    printf("║     line by line. Type a single '.' on its own line to finish.   ║\n");
    printf("║     Request:   SUBMIT report                                     ║\n");
    printf("║                This is line 1 of my document                     ║\n");
    printf("║                This is line 2 of my document                     ║\n");
    printf("║                .                                                 ║\n");
    printf("║     Response:  JOB ID: <n>                                       ║\n");
    printf("║                                                                  ║\n");
    printf("║  3. STATUS <job_id>                                              ║\n");
    printf("║     Check the current status of a print job.                     ║\n");
    printf("║     Request:   STATUS 1                                          ║\n");
    printf("║     Response:  JOB 1: QUEUED | PRINTING | DONE | CANCELED        ║\n");
    printf("║                                                                  ║\n");
    printf("║  4. LIST                                                         ║\n");
    printf("║     List all print jobs and their statuses.                      ║\n");
    printf("║     Request:   LIST                                              ║\n");
    printf("║     Response:  JOB <id>: <title> <state>  (for each job)         ║\n");
    printf("║                                                                  ║\n");
    printf("║  5. CANCEL <job_id>                                              ║\n");
    printf("║     Cancel a queued or printing job.                             ║\n");
    printf("║     Request:   CANCEL 1                                          ║\n");
    printf("║     Response:  400: JOB 1 CANCELED                               ║\n");
    printf("║                                                                  ║\n");
    printf("║  6. QUIT                                                         ║\n");
    printf("║     Disconnect from the server gracefully.                       ║\n");
    printf("║     Request:   QUIT                                              ║\n");
    printf("║     Response:  200: GOODBYE                                      ║\n");
    printf("║                                                                  ║\n");
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║  JOB LIFECYCLE:                                                  ║\n");
    printf("║     QUEUED  ──>  PRINTING  ──>  DONE                             ║\n");
    printf("║       │              │                                           ║\n");
    printf("║       └── CANCEL ────┘                                           ║\n");
    printf("║                                                                  ║\n");
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║  ERROR RESPONSES:                                                ║\n");
    printf("║  ─────────────────────────────────────────────────────────────── ║\n");
    printf("║  • 400: BAD REQUEST       ── Unknown or malformed command        ║\n");
    printf("║  • JOB <id> NOT FOUND     ── Invalid job ID in STATUS/CANCEL     ║\n");
    printf("║  • 409: JOB <id> ALREADY DONE ── Cannot cancel a finished job    ║\n");
    printf("║                                                                  ║\n");
    printf("╠══════════════════════════════════════════════════════════════════╣\n");
    printf("║  Type HELP at any time to see this reference again.              ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

int main(int argc, char *argv[]){
    if(argc!=3){
        printf("Server IP and PORT Required\n");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT,sigint_handler);

    char *serverip=argv[1];
    char *port=argv[2];

    int sockfd;
    struct sockaddr_in server_addr;

    int PORT=atoi(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(serverip);

    connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
    printf("CLIENT CONNECTED...\n");
    print_help();
    printf("quickprint> ");
    fflush(stdout);

    while(1){
        char buffer[1024];
        fgets(buffer,sizeof(buffer),stdin);
        buffer[strcspn(buffer,"\n")]='\0';

        if(strcasecmp(buffer,"HELP")==0){
            print_help();
            printf("quickprint> ");
            fflush(stdout);
            continue;
        }

        write(sockfd, buffer, strlen(buffer)+1);

        char *CMD=strtok(buffer," ");
        if(CMD==NULL){
            printf("quickprint> ");
            fflush(stdout);
            continue;
        }

        if(strcmp(CMD,"SUBMIT")==0){
            printf("  Enter document lines (type '.' on a blank line to finish):\n");
            while(1){
                char doc[4096]={0};
                int amount=0;
                
                printf("  > ");
                fflush(stdout);
                fgets(doc,sizeof(doc),stdin);
                doc[strcspn(doc,"\n")]='\0';

                write(sockfd, doc, strlen(doc)+1);

                if(strcmp(doc,".")==0){
                    break;
                }
            }
        }

        if(strcmp(CMD,"LIST")==0){
            char listbuf[8192]={0};
            read(sockfd, listbuf, sizeof(listbuf));
            printf("SERVER:\n%s",listbuf);
            printf("quickprint> ");
            fflush(stdout);
            continue;
        }

        char msg[1024];
        socklen_t len = sizeof(server_addr);
        read(sockfd, msg, sizeof(msg));
        msg[strcspn(msg,"\n")]='\0';

        printf("SERVER: %s\n",msg);

        if(strcmp(CMD,"QUIT")==0){
            close(sockfd);
            return 0;
        }

        printf("quickprint> ");
        fflush(stdout);
    }

    close(sockfd);
    return 0;
}