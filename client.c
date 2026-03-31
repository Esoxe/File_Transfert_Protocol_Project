        #include "csapp.h"
        #include "requete.h"
        #include "sys/time.h" //Utilise pour la stat de temps

        #define CLIENT_DIR "./FichierClient"


        typereq_t traduction_type_requete(char *demande){
            if(strcmp(demande,"GET")==0){
                return GET;
            }
            if(strcmp(demande,"BYE")==0){
                return BYE;
            }
            if(strcmp(demande,"LS")==0){
            return LS;
            }
            if(strcmp(demande,"RM")==0){
                return RM;
            }
            if(strcmp(demande,"PUT")==0){
                return PUT;
            }
            else{
                return -1;
            }
        }
        //Effectue une demande d'un nouveau serveur au maitre renvoie un numero de socket
        //Prend en argument l'ancien serveur ou il etait connecter 0 sinon
        int connexion_maitre(char *host,typereq_maitre_t type,int *port_connexion){
            int port = 2121;
            response_maitre_t info_serveur;
            requete_maitre_t req_maitre;
            int client_maitrefd = open_clientfd(host, port);
            if(client_maitrefd==-1){
                printf("Désolé Le serveur maitre n'est pas accessible \n");
                exit(1);
            }
            req_maitre.type=htonl((uint32_t)type); //On force le cast du type enumerer en int32
            req_maitre.port=htonl(*port_connexion);
            rio_writen(client_maitrefd,&req_maitre,sizeof(req_maitre));
            //Recupere par le maitre les infos du serveur ou il doit se connecter
            rio_readn(client_maitrefd,&info_serveur,sizeof(info_serveur));
            info_serveur.port=ntohl(info_serveur.port);
            if(info_serveur.port==-1){
                printf("Désolé tous les serveurs sont actuellement indisponibles \n");
                exit(1);
            }
            *port_connexion=info_serveur.port;
            int clientfd=open_clientfd(info_serveur.ip,info_serveur.port);
            // Cas ou le serveur est mort pendant le voyage sur le reseau des infos du maitre
            if(clientfd==-1){
                close(client_maitrefd);
                return connexion_maitre(host,PANNE,port_connexion);
            }
            close(client_maitrefd); 
            return clientfd;  
        }


        int main(int argc, char **argv)
        {
            struct timeval debut; //Gestion du temps prit par la requete
            struct timeval fin;
            double temp_ecouler;
            int clientfd;
            int port_serveur;
            char *host;
            request_t req;
            response_t rep;
            char *buf;
            int nb_recu;
            int nb_lu;
            int restant=0;
            int taille_bloc;
            char recupin[MAXLINE+256];
            char demande[MAXLINE];
            int terminer=0;
            char nom_fichier[MAXLINE];
            char chemin_local[MAXLINE+256];//Espace en plus poour le nom de dossier
            Signal(SIGPIPE,SIG_IGN);
            typereq_t type;
            struct stat st;
            int fd_res;
            if (argc != 2) {
                fprintf(stderr, "usage: %s <host>\n", argv[0]);
                exit(0);
            }
            host = argv[1];
            /*
                Demande la connexion a un serveur du maitre
            */
            clientfd=connexion_maitre(host,NOUVELLE,&port_serveur);
            /*
            * At this stage, the connection is established between the client
            * and the server OS ... but it is possible that the server application
            * has not yet called "Accept" for this connection
            */
            printf("client connected to server OS\n"); 
            
            while (!terminer)
            {
                // recup la ligne de commande
                fgets(recupin,MAXLINE+256,stdin);

                // remplissage de demande et nom du fichier
                int i=0;
                while (recupin[i]!=' ' && recupin[i]!='\0' && recupin[i]!='\n')
                {
                    demande[i]=recupin[i];
                    i++;
                }
                demande[i]='\0';
                if (recupin[i]==' ')
                {
                    i++;
                }
                
                // si il y a le nom du fichier
                if (recupin[i]!=' ' && recupin[i]!='\0' && recupin[i]!='\n')
                {
                    int j=0;
                    while (recupin[i]!='\0' && recupin[i]!='\n')
                    {
                        nom_fichier[j]=recupin[i];
                        j++;
                        i++;
                    }
                    nom_fichier[j]='\0';
                } else {
                    nom_fichier[0]='\0';
                }

                type=traduction_type_requete(demande);
                if(type==-1){
                    printf("Type de requete non definie\n");
                    continue;
                }
                req.type=htonl(type);
                //On verifie si le fichier n'est pas déja sur le pc et si il est on verifie qu'il n'a pas etait modifie par le serveur depuis
                snprintf(chemin_local,MAXLINE+256,"%s/%s",CLIENT_DIR,nom_fichier);
                if(stat(chemin_local,&st)==-1){
                    if (type == PUT)
                    {
                        printf("Erreur: le fichier n'existe pas en locale\n");
                        continue;
                    }
                    req.octets_deja_recu=0;
                    req.date_fichier=0;
                }
                else{
                    req.octets_deja_recu=htonl(st.st_size);
                    restant=-st.st_size;
                    char chemin_info[MAXLINE];//Envoie la derniere date de modif de ce fichier par le serveur
                    snprintf(chemin_info,MAXLINE+256,"%s/.%s.info",CLIENT_DIR,nom_fichier);
                    int fd_info=open(chemin_info,O_RDONLY);
                    if(fd_info!=-1){
                        rio_readn(fd_info,&req.date_fichier,sizeof(uint32_t));
                    }
                    else {req.date_fichier=0;}
                    close(fd_info);
                }
                strcpy(req.nom_ficher,nom_fichier);
                if (type==PUT)
                {
                    req.taille_fichier=htonl(st.st_size);
                }
                req.date_fichier=(htonl(req.date_fichier));
                int writ_t = rio_writen(clientfd,&req,sizeof(req));
                int read_t=rio_readn(clientfd,&rep,sizeof(rep));
                //Si le serveur a crash pendant attente du client on refait une demande au maitre
                if(writ_t==-1 || read_t<=0){
                    clientfd=connexion_maitre(host,PANNE,&port_serveur);
                    rio_writen(clientfd,&req,sizeof(req));
                    rio_readn(clientfd,&rep,sizeof(rep));
                }
                //Convertie dans l'architecture du client
                rep.code_retour=ntohl(rep.code_retour);
                switch (rep.code_retour)
                {
                case SUCCES:
                    printf("Le fichier a bien été supprimé\n");
                    break;
                case FICHIER_NON_TROUVE:
                    printf("Le fichier n'est pas dans le serveur\n");
                    break;
                case ENVOIE_COMPLET :
                case ENVOIE_PARTIEL :
                    rep.taille_bloc=ntohl(rep.taille_bloc);
                    rep.taille_fichier=ntohl(rep.taille_fichier);
                    rep.date_modif=ntohl(rep.date_modif);
                    //Garde en mémoire la date du fichier du serveur permet en cas de crash de verifier si le fichier source n'a pas changé
                    char chemin_info[MAXLINE];
                    snprintf(chemin_info,MAXLINE+256,"%s/.%s.info",CLIENT_DIR,nom_fichier);
                    int fd_info=open(chemin_info,O_CREAT | O_WRONLY | O_TRUNC,0644);
                    rio_writen(fd_info,&rep.date_modif,sizeof(uint32_t));
                    req.date_fichier=htonl(rep.date_modif);
                    close(fd_info);
                    //Cas une partie du fichier est déja téléchargé
                    if(rep.code_retour==ENVOIE_PARTIEL){
                        fd_res=open(chemin_local,O_WRONLY | O_APPEND,0644);
                    }
                    else{//Cas fichier n'existe pas
                        fd_res=open(chemin_local,O_CREAT | O_WRONLY | O_TRUNC,0644);
                    }
                    restant=rep.taille_fichier;
                    taille_bloc=rep.taille_bloc;
                    buf=malloc(taille_bloc);
                    if(buf==NULL){//Erreur d'allocation mémoire
                        break;
                    }
                    int totale_recuperer=0;
                    int memoire_octets_initiaux = ntohl(req.octets_deja_recu);//On reconvertie car convertie pour envoie
                    gettimeofday(&debut,NULL);//Lance le chrono
                    while((restant>0))
                    {   
                        if(restant>taille_bloc)
                            nb_recu = rio_readn(clientfd,buf,taille_bloc);
                        else{
                            nb_recu = rio_readn(clientfd,buf,restant);
                        }
                        if(nb_recu<=0){
                            //Le serveur a crash on va en demander un nouveau au maitre
                            clientfd=connexion_maitre(host,PANNE,&port_serveur);
                            //On refait une demande au nouveau serveur et on recupere ses information puis on relance telechargement
                            req.octets_deja_recu=htonl(memoire_octets_initiaux+totale_recuperer);//Nombre d'octets initaux puis ceux avant crash
                            rio_writen(clientfd,&req,sizeof(req));
                            rio_readn(clientfd,&rep,sizeof(rep));
                            int nouveau_code=ntohl(rep.code_retour);
                            if(nouveau_code==ENVOIE_COMPLET){//Si jamais le fichier est mis a jour pendant le telechargement
                                ftruncate(fd_res,0);
                                totale_recuperer=0;
                            }
                            if(nouveau_code==FICHIER_NON_TROUVE){//SI le nouveau serveur n'a pas le fichier l'utilisateur doit refaire une demande
                                printf("Erreur critique : le serveur de secours ne poosède pas le fichier.\n");
                                break;
                            }
                            //Pour etre sur de syncroniser le serveur et le client mais normalement inutile
                            restant=ntohl(rep.taille_fichier);
                            continue;
                        }
                        rio_writen(fd_res,buf,nb_recu);
                        totale_recuperer+=nb_recu;
                        restant-=nb_recu;
                    }
                    gettimeofday(&fin,NULL);//Fin chrono
                    temp_ecouler=(fin.tv_sec-debut.tv_sec)+(fin.tv_usec-debut.tv_usec)/1000000.0;
                    if(temp_ecouler>0){printf("Fichier %s bien reçu, %d octets en %f secondes (%f Kbytes/s)\n",req.nom_ficher,totale_recuperer,temp_ecouler,(totale_recuperer/1024.0)/temp_ecouler);}
                    else{printf("Fichier %s bien reçu, %d octets en %f secondes\n",req.nom_ficher,totale_recuperer,temp_ecouler);}
                    free(buf);    
                    close(fd_res);
                    break;
                case DEJA_COMPLET:
                    printf("Le fichier %s est déja complet et a jour sur le disque local\n",req.nom_ficher);
                    break;
                case ENVOIE_LS:
                    int taille_ls=ntohl(rep.taille_fichier);
                    buf=malloc(taille_ls);
                    rio_readn(clientfd,buf,taille_ls);
                    rio_writen(STDOUT_FILENO,buf,taille_ls);
                    free(buf);
                    break;
                case ERREUR_LS:
                    printf("Erreur lors de l'éxécution de la commande LS\n");
                    break;
                case READY_PUT:
                    int fd=open(chemin_local,O_RDONLY,0);
                    restant=st.st_size;
                    taille_bloc=ntohl(rep.taille_bloc);
                    buf=malloc(taille_bloc);
                    int total_envoye=0;

                    while (restant>0)
                    {
                        if(restant>taille_bloc)
                            nb_lu = rio_readn(fd,buf,taille_bloc);
                        else{
                            nb_lu = rio_readn(fd,buf,restant);
                        }

                        rio_writen(clientfd,buf,nb_lu);
                        total_envoye+=nb_lu;
                        restant-=nb_lu;
                    }
                    printf("Fichier %s bien envoyé, (%d octets) \n",req.nom_ficher,total_envoye);
                    free(buf);
                    close(fd);
                    break;
            case FIN_CONNEXION:
                    printf("Goodbye\n");
                    terminer=1;
                    break;
                default:
                    printf("Erreur coté serveur\n");
                    break;
                }
            }
            Close(clientfd);
            exit(0);
        }
