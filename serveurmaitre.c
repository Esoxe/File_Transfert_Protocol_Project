#include "serveurmaitre.h"
#define MAX_NAME_LEN 256
//On suppose ici que les serveur esclave son sur la même machine donc on utilise l'ip 127.0.0.1
//Sinon on definira un tableau d'adresse IP pour chaque serveur 
//Si les serveurs sont sur des machines differentes on pourra utiliser le même port pour toute


int main(int argc, char **argv){
    info_esclave_t tab_esclaves[NB_SLAVES];
    //On Se connecte et verifie la liste de serveur disponible
    for(int i=0;i<NB_SLAVES;i++){
        char *current_ip=CONFIG_SERVEURS[i].ip;
        int current_port=CONFIG_SERVEURS[i].port;
        int serveurfd = open_clientfd(current_ip,current_port);
        if(serveurfd==-1){
            printf("Serveur numero %d au port %d n'est pas disponible\n",i,current_port);
            tab_esclaves[i].port=-1; //Serveur non disponible
        }
        else{
            strcpy(tab_esclaves[i].ip,current_ip);
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
            req_maitre.port=ntohl(req_maitre.port);//Convertion boutisme du serveur
            req_maitre.type=(typereq_maitre_t)ntohl((uint32_t)req_maitre.type);//Cast au cas ou le type enuméré est pas en int32
            //Detecte si c'est une nouvelle demande ou non si non elle met a jour l'état du serveur defectueux
            if(req_maitre.type==PANNE){
                //On cherche l'esclaves dans l'annuaire
                for(int j=0; j<NB_SLAVES; j++){
                    if(CONFIG_SERVEURS[j].port == req_maitre.port){
                        tab_esclaves[j].port = -1;
                        break;
                    }
                }
            }
            //Trouve le premier serveur disponible
            int i = 0;
            for(i=0;i<NB_SLAVES;i++)
            {
                if(tab_esclaves[tourniquet].port!=-1){
                    int test=open_clientfd(tab_esclaves[tourniquet].ip,tab_esclaves[tourniquet].port);
                    if(test==-1){
                        tab_esclaves[tourniquet].port=-1;
                    }
                    else{
                        close(test);
                        break;
                    }
                }
                tourniquet=(tourniquet+1)%NB_SLAVES;
            }
            if(i==NB_SLAVES){//Cas ou aucun serveur n'est disponible
                rep.port=htonl(-1);
            }
            else{
                strcpy(rep.ip,tab_esclaves[tourniquet].ip);
                rep.port=htonl(tab_esclaves[tourniquet].port);
            }
            rio_writen(connfd,&rep,sizeof(rep));
            if(i!=NB_SLAVES){
                printf("Client (%s,ip:%s) connecter au serveur n° %d d'adresse %s et au port %d\n",client_hostname,client_ip_string,tourniquet,tab_esclaves[tourniquet].ip,tab_esclaves[tourniquet].port); 
            }
            else{
                printf("Aucun serveur disponible relancer les serveurs et le serveur maitre\n");
            }
            tourniquet=(tourniquet+1)%NB_SLAVES;
            printf("server Master disconnected to %s (%s)\n", client_hostname,
            client_ip_string); 
            Close(connfd);
        }
    }
    

}