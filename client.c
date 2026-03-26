/*
 * echoclient.c - An echo client
 */
#include "csapp.h"
#include "requete.h"
#include "sys/time.h" //Utilise pour la stat de temps

#define CLIENT_DIR "./FichierClient"


typereq_t traduction_type_requete(char *demande){
    if(strcmp(demande,"GET")==0){
        return GET;
    }
    else{
        return -1;
    }
}



int main(int argc, char **argv)
{
    struct timeval debut; //Gestion du temps prit par la requete
    struct timeval fin;
    double temp_ecouler;
    int clientfd, port = 2121;
    char *host;
    request_t *req = malloc(sizeof(*req));
    response_t *rep = malloc(sizeof(*rep));
    char * fichier ;
    char demande[MAXLINE];
    char nom_fichier[MAXLINE];
    typereq_t type;
    int fd_res;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <host>", argv[0]);
        exit(0);
    }
    host = argv[1];
    /*
     * Note that the 'host' can be a name or an IP address.
     * If necessary, Open_clientfd will perform the name resolution
     * to obtain the IP address.
     */
    clientfd = Open_clientfd(host, port);
    
    /*
     * At this stage, the connection is established between the client
     * and the server OS ... but it is possible that the server application
     * has not yet called "Accept" for this connection
     */
    printf("client connected to server OS\n"); 
    
    scanf("%s %s",demande,nom_fichier);
    type=traduction_type_requete(demande);
    if(type==-1){
        printf("Type de requete non definie\n");
        free(rep);
        free(req);
        close(clientfd);
        exit(0);
    }
    req->type=type;
    strcpy(req->nom_ficher,nom_fichier);
    rio_writen(clientfd,req,sizeof(*req));
    rio_readn(clientfd,rep,sizeof(*rep));
    switch (rep->code_retour)
    {
    case 404:
        printf("Le fichier n'est pas dans le serveur\n");
        break;
    case 0 :
        fichier=malloc(rep->taille_fichier);
        gettimeofday(&debut,NULL);//Lance le chrono
        rio_readn(clientfd,fichier,rep->taille_fichier);
        gettimeofday(&fin,NULL);//Fin chrono
        temp_ecouler=(fin.tv_sec-debut.tv_sec)+(fin.tv_usec-debut.tv_usec)/1000000.0;
        printf("Fichier %s bien reçu, %d octets en %f secondes (%f Kbytes/s)\n",req->nom_ficher,rep->taille_fichier,temp_ecouler,(rep->taille_fichier/1024)/temp_ecouler);
        //Marge de sécurité de 256 pour le nom de dossier
        snprintf(nom_fichier,MAXLINE+256,"%s/%s",CLIENT_DIR,req->nom_ficher);
        fd_res=open(nom_fichier,O_CREAT | O_WRONLY | O_TRUNC,0644);
        rio_writen(fd_res,fichier,rep->taille_fichier);
        close(fd_res);
        free(fichier);
        break;
    default:
        printf("Erreur coté serveur\n");
        break;
    }
    free(rep);
    free(req);
    Close(clientfd);
    exit(0);
}
