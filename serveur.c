#include "serveurmaitre.h"
#define NB_PROCS 10 
#define MAX_NAME_LEN 256
#define SERVER_DIR "./FichierServeur"
#define CLIENT_DIR "./FichierClient"
#define TAILLE_BLOC 8192


void handler(int sig) {
    Kill(-getpid(),SIGKILL);
    exit(0);
}
void traitement_serveur(int connfd){
    request_t req;
    response_t rep;
    char nom_fichier[MAXLINE+256]; //Marge de sécurité avec l'ajout du dossier
    char buf[TAILLE_BLOC];
    int fd;
    int nb_lue;
    while (rio_readn(connfd, &req, sizeof(req))>0)
    {
        switch (req.type)
        {
        case GET:
            snprintf(nom_fichier,MAXLINE + 256,"%s/%s",SERVER_DIR,req.nom_ficher);
            fd=open(nom_fichier,O_RDONLY,0);
            if(fd==-1){
                rep.code_retour=404;
                rio_writen(connfd,&rep,sizeof(rep));
            }
            else{            
                struct stat st;
                stat(nom_fichier,&st);
                if(req.octets_deja_recu!=0 && req.date_fichier==st.st_mtime){
                    //Si le fichier du serveur n'a pas était modifié depuis le derniere telechargment on ne commence pas du début
                    if(req.octets_deja_recu==st.st_size){//Cas ou le fichier est complet
                        rep.code_retour=DEJA_COMPLET;
                        rep.taille_fichier=0;
                    }
                    else{
                        lseek(fd,req.octets_deja_recu,SEEK_SET);
                        rep.taille_fichier=st.st_size-req.octets_deja_recu;
                        rep.code_retour=ENVOIE_PARTIEL;        
                    }
                }
                else{
                    rep.taille_fichier=st.st_size;
                    rep.code_retour=ENVOIE_COMPLET;
                }
                rep.date_modif=st.st_mtime; //Recupere la date de derniere modification du fichier
                rep.taille_bloc=TAILLE_BLOC;
                rio_writen(connfd,&rep,sizeof(rep));
                if(rep.taille_fichier >0){
                    while((nb_lue=rio_readn(fd,buf,TAILLE_BLOC))>0)
                    {
                        int statut_client=rio_writen(connfd,buf,nb_lue);
                        if(statut_client==-1){break;}
                        // usleep(50000); //Pour débugage commenter si pas fait
                    }
                }
                close(fd);
            }           
            break;
        case LS:
            // le resultat de ls sera dans fpipe
            FILE * fpipe = popen("ls -1 ./FichierServeur","r");
            if (fpipe==NULL)
            {
                rep.code_retour=237;
                rio_writen(connfd,&rep,sizeof(rep));

            } else {
                char buffer[50000];
                int taille_ls=fread(buffer, sizeof(char), 50000, fpipe);
                rep.taille_fichier=taille_ls;
                rep.code_retour=236;
                rep.taille_bloc=TAILLE_BLOC;
                rio_writen(connfd,&rep,sizeof(rep));
                rio_writen(connfd,buffer,taille_ls);
                pclose(fpipe);
            }
            break;
        case RM:
            snprintf(nom_fichier,MAXLINE + 256,"%s/%s",SERVER_DIR,req.nom_ficher);
            int ret=remove(nom_fichier);
            if (ret==0)
            {
                rep.code_retour=0;
            } else {
                rep.code_retour=1;
            }
            rio_writen(connfd,&rep,sizeof(rep));
            break;
        case PUT:
            snprintf(nom_fichier,MAXLINE + 256,"%s/%s",SERVER_DIR,req.nom_ficher);
            rep.taille_bloc=TAILLE_BLOC;
            rep.code_retour=READY_PUT;
            fd=open(nom_fichier, O_CREAT | O_WRONLY | O_TRUNC,0644);
            rio_writen(connfd,&rep,sizeof(rep));
            int total_recu=0;
                if(req.taille_fichier >0){
                    int restant=req.taille_fichier;
                    char *buf=malloc(TAILLE_BLOC);
                    int nb_recu;

                    while (restant>0)
                    {
                        if(restant>TAILLE_BLOC)
                            nb_recu = rio_readn(connfd,buf,TAILLE_BLOC);
                        else{
                            nb_recu = rio_readn(connfd,buf,restant);
                        }

                        rio_writen(fd,buf,nb_recu);
                        total_recu+=nb_recu;
                        restant-=nb_recu;
                    }
                    free(buf);
                }
                printf("Fichier %s bien recu, (%d octets) \n",req.nom_ficher,total_recu);
                close(fd);
            break;
        case BYE:
            rep.code_retour=67;
            rio_writen(connfd,&rep,sizeof(rep));
            break;
        default:        
            break;
        }
    }
}




int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>(entre %d et %d)\n", argv[0],PORT_DEBUT_ESCLAVE,PORT_DEBUT_ESCLAVE+NB_SLAVES);
        exit(0);
    }
    int listenfd, connfd, port=atoi(argv[1]);
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

