#include "csapp.h"
#define NB_PROCS  10
#define MAX_NAME_LEN 256

void handler(int sig) {
    Kill(-getpid(),SIGINT);
    exit(0);
}


int main(int argc, char **argv)
{
    int listenfd, connfd, port=2121;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    
    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(port);
        for(int i =0;i<NB_PROCS;i++){
            if(Fork==0){
                while (1){
                {
                    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
                    if(connfd!=-1){
                        /* determine the name of the client */
                        Getnameinfo((SA *) &clientaddr, clientlen,
                        client_hostname, MAX_NAME_LEN, 0, 0, 0);
            
                        /* determine the textual representation of the client's IP address */
                        Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                        INET_ADDRSTRLEN);
            
                        printf("server connected to %s (%s)\n", client_hostname,
                        client_ip_string); 
                        while(1){}
                        exit(0);      
                        // Close(connfd);
                    }
                }
            }
        }
        while (1) {}
        exit(0);
    }
}

