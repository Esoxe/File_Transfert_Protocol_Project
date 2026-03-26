#include "csapp.h"
#include "requete.h"
#define NB_PROCS 10 
#define MAX_NAME_LEN 256
#define SERVER_DIR "./FichierServeur"

void handler(int sig) {
    Kill(-getpid(),SIGINT);
    exit(0);
}
void traitement_serveur(int connfd){
    request_t *req = malloc(sizeof(*req));
    response_t *rep = malloc(sizeof(*rep));
    char nom_fichier[MAXLINE+256]; //Marge de sécurité avec l'ajout du dossier
    char * fichier;
    int fd;
    rio_readn(connfd, req, sizeof(*req));
    switch (req->type)
    {
    case GET:
    
        snprintf(nom_fichier,MAXLINE + 256,"%s/%s",SERVER_DIR,req->nom_ficher);
        fd=open(nom_fichier,O_RDONLY,0);
        if(fd==-1){
            printf("Le fichier n'est pas sur le serveur\n");
            rep->code_retour=404;
            rio_writen(connfd,rep,sizeof(*rep));
        }
        else{
            rep->code_retour = 0;
            struct stat st;
            stat(nom_fichier,&st);
            rep->taille_fichier=st.st_size;
            fichier=malloc(st.st_size);
            rio_readn(fd,fichier,st.st_size);
            rio_writen(connfd,rep,sizeof(*rep));
            rio_writen(connfd,fichier,st.st_size);
            close(fd);
            free(fichier);
        }
        free(req);
        free(rep);
        break;
    default:        
        free(req);
        free(rep);
        break;
    }
}




int main(int argc, char **argv)
{
    int listenfd, connfd, port=2121;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    
    clientlen = (socklen_t)sizeof(clientaddr);

    listenfd = Open_listenfd(port);
    for(int i =0;i<NB_PROCS;i++)
    {
        if(Fork()==0)
        {
            while (1)
            {
                connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
                if(connfd!=-1)
                {
                    /* determine the name of the client */
                    Getnameinfo((SA *) &clientaddr, clientlen,
                    client_hostname, MAX_NAME_LEN, 0, 0, 0);
        
                    /* determine the textual representation of the client's IP address */
                    Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
                    INET_ADDRSTRLEN);
        
                    printf("server connected to %s (%s)\n", client_hostname,
                    client_ip_string); 
                    traitement_serveur(connfd);
                    Close(connfd);
                }
            }
        }        
    }
    while (1) {}
}

