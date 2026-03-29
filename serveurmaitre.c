#include "serveurmaitre.h"
#define MAX_NAME_LEN 256

//On suppose ici que les serveur esclave son sur la même machine donc on utilise l'ip 127.0.0.1
//Sinon on definira un tableau d'adresse IP pour chaque serveur 
//Si les serveurs sont sur des machines differentes on pourra utiliser le même port pour toute


int main(int argc, char **argv){
    info_esclave_t tab_esclaves[NB_SLAVES];
    //On Se connecte et verifie la liste de serveur disponible
    for(int i=0;i<NB_SLAVES;i++){
        int current_port=PORT_DEBUT_ESCLAVE+i;
        int serveurfd = open_clientfd("127.0.0.1",current_port);
        if(serveurfd==-1){
            printf("Serveur numero %d au port %d n'est pas disponible",i,current_port);
            tab_esclaves[i].port=-1; //Serveur non disponible
        }
        else{
            strcpy(tab_esclaves[i].ip,"127.0.0.1");
            tab_esclaves[i].port=current_port;
            close(serveurfd);
        }
    }
    int listenfd = Open_listenfd(PORT_MAITRE);
    struct sockaddr_in clientaddr;
    socklen_t clientlen;
    char client_hostname[MAX_NAME_LEN];
    char client_ip_string[INET_ADDRSTRLEN];
    int tourniquet = 0;
    requete_maitre_t req_maitre;
    response_maitre_t rep;
    while (1)
    {
        int connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        if(connfd!=-1)
        {
            /* determine the name of the client */
            Getnameinfo((SA *) &clientaddr, clientlen,
            client_hostname, MAX_NAME_LEN, 0, 0, 0);

            /* determine the textual representation of the client's IP address */
            Inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip_string,
            INET_ADDRSTRLEN);

            printf("server Master connected to %s (%s)\n", client_hostname,
            client_ip_string); 
            rio_readn(connfd,&req_maitre,sizeof(req_maitre));
            //Detecte si c'est une nouvelle demande ou non si non elle met a jour l'état du serveur defectueux
            if(req_maitre.type==PANNE){
                //On verifie que le numéro de port envoyer a du sens
                if(req_maitre.port>=PORT_DEBUT_ESCLAVE && req_maitre.port<PORT_DEBUT_ESCLAVE+NB_SLAVES){
                    tab_esclaves[req_maitre.port-PORT_DEBUT_ESCLAVE].port=-1;
                }
            }
            //Trouve le premier serveur disponible
            int i=0;
            while (tab_esclaves[tourniquet].port==-1&& i<NB_SLAVES){
                tourniquet=(tourniquet+1)%NB_SLAVES;
                i++;
            }
            if(i==NB_SLAVES){//Cas ou aucun serveur n'est disponible
                rep.port=-1;
            }
            else{
                strcpy(rep.ip,tab_esclaves[tourniquet].ip);
                rep.port=tab_esclaves[tourniquet].port;
            }
            rio_writen(connfd,&rep,sizeof(rep));
            printf("Client (%s,ip:%s) connecter au serveur n° %d d'adresse %s et au port %d\n",client_hostname,client_ip_string,tourniquet,tab_esclaves[tourniquet].ip,tab_esclaves[tourniquet].port); 
            tourniquet=(tourniquet+1)%NB_SLAVES;
            Close(connfd);
        }
    }
    

}