#include "serveurmaitre.h"
#define NB_PROCS 10 
#define MAX_NAME_LEN 256
#define SERVER_DIR "./FichierServeur"
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
        //Adaptation a l'architecture du serveur
        req.type=ntohl(req.type);
        switch (req.type)
        {
        case GET:
            snprintf(nom_fichier,MAXLINE + 256,"%s/%s",SERVER_DIR,req.nom_ficher);
            fd=open(nom_fichier,O_RDONLY,0);
            if(fd==-1){
                rep.code_retour=htonl(404);
                rio_writen(connfd,&rep,sizeof(rep));
            }
            else{         
                req.date_fichier=ntohl(req.date_fichier);
                req.octets_deja_recu=ntohl(req.octets_deja_recu);   
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
                //Prepare l'envoie en adaptant l'architecture
                rep.taille_bloc=htonl(rep.taille_bloc);
                int taille_a_envoyer=rep.taille_fichier;
                rep.taille_fichier=htonl(rep.taille_fichier);
                rep.code_retour=htonl(rep.code_retour);
                rep.date_modif=htonl(rep.date_modif);
                rio_writen(connfd,&rep,sizeof(rep));
                if(taille_a_envoyer >0){
                    while((nb_lue=rio_readn(fd,buf,TAILLE_BLOC))>0)
                    {
                        int statut_client=rio_writen(connfd,buf,nb_lue);
                        if(statut_client==-1){break;}
                        // usleep(5000); //Pour débugage commenter si pas fait
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
                rep.code_retour=htonl(237);
                rio_writen(connfd,&rep,sizeof(rep));

            } else {
                char buffer[50000];
                int taille_ls=fread(buffer, sizeof(char), 50000, fpipe);
                rep.taille_fichier=htonl(taille_ls);
                rep.code_retour=htonl(236);
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
                rep.code_retour=htonl(0);
            } else {
                rep.code_retour=htonl(404);
            }
            rio_writen(connfd,&rep,sizeof(rep));
            break;
        case PUT:
            req.taille_fichier=ntohl(req.taille_fichier);
            snprintf(nom_fichier,MAXLINE + 256,"%s/%s",SERVER_DIR,req.nom_ficher);
            rep.taille_bloc=htonl(TAILLE_BLOC);
            rep.code_retour=htonl(READY_PUT);
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
            memset(&rep,0,sizeof(rep));
            rep.code_retour=htonl(67);
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
    int port=atoi(argv[1]);
    //On verifie si le numéro de port est disponible et le numéro valide
    int test = -1;
    while ((port<PORT_DEBUT_ESCLAVE || port>=PORT_DEBUT_ESCLAVE+NB_SLAVES) || ((test=open_listenfd(port))==-1))
    {
        printf("Port non disponible ou en dehors de la plage %d-%d : ",PORT_DEBUT_ESCLAVE,PORT_DEBUT_ESCLAVE+NB_SLAVES);
        scanf("%d",&port);
    }
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    
    clientlen = (socklen_t)sizeof(clientaddr);
    Signal(SIGINT,handler);
    listenfd = test;
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
    for(int i =0;i<NB_PROCS;i++){
        Wait(NULL);
    }
}

