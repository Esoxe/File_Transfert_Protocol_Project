#include "csapp.h"
#include "requete.h"
#define NB_PROCS 10 
#define MAX_NAME_LEN 256
#define SERVER_DIR "./FichierServeur"
#define TAILLE_BLOC 8192


void handler(int sig) {
    Kill(-getpid(),SIGKILL);
    exit(0);
}
void traitement_serveur(int connfd){
    request_t *req = malloc(sizeof(*req));
    response_t *rep = malloc(sizeof(*rep));
    char nom_fichier[MAXLINE+256]; //Marge de sécurité avec l'ajout du dossier
    char buf[TAILLE_BLOC];
    int fd;
    int nb_lue;
    while (rio_readn(connfd, req, sizeof(*req))>0)
    {
        switch (req->type)
        {
        case GET:
            snprintf(nom_fichier,MAXLINE + 256,"%s/%s",SERVER_DIR,req->nom_ficher);
            fd=open(nom_fichier,O_RDONLY,0);
            if(fd==-1){
                rep->code_retour=404;
                rio_writen(connfd,rep,sizeof(*rep));
            }
            else{            
                struct stat st;
                stat(nom_fichier,&st);
                if(req->octets_deja_recu!=0 && req->date_fichier==st.st_mtime){
                    //Si le fichier du serveur n'a pas était modifié depuis le derniere telechargment on ne commence pas du début
                    lseek(fd,req->octets_deja_recu,SEEK_SET);
                    rep->taille_fichier=st.st_size-req->octets_deja_recu;
                    rep->code_retour=ENVOIE_PARTIEL;        
                }
                else{
                    rep->taille_fichier=st.st_size;
                    rep->code_retour=ENVOIE_COMPLET;
                }
                rep->date_modif=st.st_mtime; //Recupere la date de derniere modification du fichier
                rep->taille_bloc=TAILLE_BLOC;
                rio_writen(connfd,rep,sizeof(*rep));
                while((nb_lue=rio_readn(fd,buf,TAILLE_BLOC))>0)
                {
                    int statut_client=rio_writen(connfd,buf,nb_lue);
                    if(statut_client==-1){break;}
                    // usleep(50000); //Pour débugage commenter si pas fait
                }
                close(fd);
            }           
            free(req);
            free(rep);
            break;
        case BYE:
            rep->code_retour=67;
            rio_writen(connfd,rep,sizeof(*rep));
            free(req);
            free(rep);
            return;
            
        default:        
            break;
        }
    
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
    Signal(SIGINT,handler);
    listenfd = Open_listenfd(port);
    for(int i =0;i<NB_PROCS;i++)
    {
        if(Fork()==0)
        {
            Signal(SIGPIPE,SIG_IGN);//On ignore les sigpipe possible si le client se deconnecte durant transfert on gére manuellement
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
                    printf("server disconected to %s (%s)\n", client_hostname,client_ip_string); 

                    Close(connfd);
                }
            }
        }        
    }
    while (1) {}
}

