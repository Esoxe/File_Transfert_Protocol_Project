#include "serveurmaitre.h"
#define NB_PROCS 10 
#define MAX_NAME_LEN 256
#define SERVER_DIR "./FichierServeur_"
#define TAILLE_BLOC 8192


void handler(int sig) {
    Signal(SIGINT,SIG_IGN);
    Kill(-getpid(),SIGINT);
    while (waitpid(-1,NULL,0)>0){}
    exit(0);
}
//On suppose ici que tous les serveurs sont sur le même pc on pourrait demander au maitre qui sont tous les autres serveurs
//Pour pouvoir propager la mise a jour
void syncronisation_serveur(int current_port,typereq_t type,char *file_name,off_t size){
    request_t req;
    response_t rep;
    req.type=htonl(type);
    strcpy(req.nom_ficher,file_name);
    req.taille_fichier=htonl(size);
    req.date_fichier=htonl(0);
    char chemin_local[MAXLINE+256];
    snprintf(chemin_local, MAXLINE + 256, "./FichierServeur_%d/%s", current_port, file_name);
    char buf[TAILLE_BLOC];
    int fd=-1;
    if(type== SYNC_PUT){
        fd=open(chemin_local,O_RDONLY,0);
        if(fd==-1) return; //SI le fichier est introuvable
    }
    for(int i=0;i<NB_SLAVES;i++){
        char *target_ip=CONFIG_SERVEURS[i].ip;
        int target_port = CONFIG_SERVEURS[i].port;
        if(target_port==current_port){//On ne syncronise pas le serveur avec lui même
            continue;
        }
        int current_transfert=open_clientfd(target_ip,target_port);
        if(current_transfert!=-1){
            rio_writen(current_transfert,&req,sizeof(req));
            rio_readn(current_transfert,&rep,sizeof(rep));
            rep.code_retour=ntohl(rep.code_retour);
            if(rep.code_retour==READY_PUT){//On envoie le fichier au serveur code similaire a put client ou get serveur
                lseek(fd,0,SEEK_SET);//On reviens au début du fichier pour une nouvelle sauvegarde
                int restant=size;
                int total_envoye=0;
                int nb_lu;
                while (restant>0)
                {
                    if(restant>TAILLE_BLOC)
                        nb_lu = rio_readn(fd,buf,TAILLE_BLOC);
                    else{
                        nb_lu = rio_readn(fd,buf,restant);
                    }

                    rio_writen(current_transfert,buf,nb_lu);
                    total_envoye+=nb_lu;
                    restant-=nb_lu;
                }
                printf("Fichier %s bien envoyé, (%d octets) au serveur de port %d \n",file_name,total_envoye,current_port);
            }
            else if(rep.code_retour==SUCCES){
                printf("Fichier %s bien supprimé, au serveur de port %d \n",file_name,current_port);
            }
            close(current_transfert);
        }
    }
    if(fd!=-1){
        close(fd);
    }
}

void traitement_serveur(int connfd,int port){
    char *tab_utilisateur[3]={"vania:jadoreSR","florian:perfectionniste","ange:jadoreVania"};
    request_t req;
    response_t rep;
    char nom_fichier[MAXLINE+256]; //Marge de sécurité avec l'ajout du dossier
    char buf[TAILLE_BLOC];
    int fd;
    int nb_lue;
    int auth=0;
    while (rio_readn(connfd, &req, sizeof(req))>0)
    {
        //Adaptation a l'architecture du serveur
        req.type=ntohl(req.type);
        //Verifie si authentifie si commande qui a besoin
        if((req.type==RM || req.type==PUT) && auth==0){
            rep.code_retour=htonl(NON_AUTORISE);
            rio_writen(connfd,&rep,sizeof(rep));
            continue;
        }
        switch (req.type)
        {
        case AUTH:
            for(int i=0;i<3;i++){
                if(strcmp(tab_utilisateur[i],req.nom_ficher)==0){
                    auth=1;
                }
            }
            if(auth==1){
                rep.code_retour=htonl(AUTH_OK);
            }
            else{
                rep.code_retour=htonl(AUTH_FAILED);
            }
            rio_writen(connfd,&rep,sizeof(rep));
            break;
        case GET:
            snprintf(nom_fichier,MAXLINE + 256,"./FichierServeur_%d/%s",port,req.nom_ficher);
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
            char commande_ls[MAXLINE];
            snprintf(commande_ls,MAXLINE,"ls -1 ./FichierServeur_%d",port);
            FILE * fpipe = popen(commande_ls,"r");
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
        case SYNC_RM:
            snprintf(nom_fichier,MAXLINE + 256,"./FichierServeur_%d/%s",port,req.nom_ficher);
            int ret=remove(nom_fichier);
            if (ret==0)
            {
                rep.code_retour=htonl(SUCCES);
            } else {
                rep.code_retour=htonl(FICHIER_NON_TROUVE);
            }
            rio_writen(connfd,&rep,sizeof(rep));
            if(req.type==RM){
                syncronisation_serveur(port,SYNC_RM,req.nom_ficher,0);
            }
            break;
        case PUT:
        case SYNC_PUT:
            req.taille_fichier=ntohl(req.taille_fichier);
            snprintf(nom_fichier,MAXLINE + 256,"./FichierServeur_%d/%s",port,req.nom_ficher);
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
                if(req.type==PUT){
                    syncronisation_serveur(port,SYNC_PUT,req.nom_ficher,req.taille_fichier);
                }
            break;

        case BYE:
            memset(&rep,0,sizeof(rep));
            rep.code_retour=htonl(FIN_CONNEXION);
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
        fprintf(stderr, "usage: %s <port>(doit faire partie de l'annuaire)\n", argv[0]);
        exit(0);
    }
    int port=atoi(argv[1]);
    //On verifie si le port appartient a l'annuaire
    int port_valide = 0;
    for(int i=0; i<NB_SLAVES; i++){
        if(CONFIG_SERVEURS[i].port == port) {
            port_valide = 1;
            break;
        }
    }
    int test = -1;
    while (!port_valide || ((test=open_listenfd(port))==-1))
    {
        printf("Port non valide ou indisponible. Veuillez entrer un port de l'annuaire : ");
        scanf("%d",&port);
        port_valide = 0;
        for(int i=0; i<NB_SLAVES; i++){//verifie si il est valide
            if(CONFIG_SERVEURS[i].port == port) {
                port_valide = 1;
                break;
            }
        }
    }
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    char client_ip_string[INET_ADDRSTRLEN];
    char client_hostname[MAX_NAME_LEN];
    char chemin_local[MAXLINE+256];
    snprintf(chemin_local,MAXLINE ,"./FichierServeur_%d",port);
    mkdir(chemin_local,0777);//Crée le répertoire de ce serveur sur la machine permet de tester en locale notamment
    clientlen = (socklen_t)sizeof(clientaddr);
    Signal(SIGINT,handler);
    listenfd = test;
    for(int i =0;i<NB_PROCS;i++)
    {
        if(Fork()==0)
        {
            Signal(SIGINT,SIG_DFL);
            Signal(SIGPIPE,SIG_IGN);//On ignore les sigpipe possible si le client se deconnecte durant transfert
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
                    traitement_serveur(connfd,port);
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

