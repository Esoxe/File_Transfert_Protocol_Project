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
        else{
            return -1;
        }
    }

    int main(int argc, char **argv)
    {
        struct timeval debut; //Gestion du temps prit par la requete
        struct timeval fin;
        double temp_ecouler;
        int clientfd,client_maitrefd, port = 2121;
        char *host;
        request_t req;
        response_t rep;
        char *buf;
        int nb_recu;
        int restant=0;
        int taille_bloc;
        char recupin[MAXLINE+256];
        char demande[MAXLINE];
        int terminer=0;
        char nom_fichier[MAXLINE];
        char chemin_local[MAXLINE+256];//Espace en plus poour le nom de dossier

        response_maitre_t info_serveur;
        requete_maitre_t req_maitre;
        typereq_t type;
        struct stat st;
        int fd_res;
        if (argc != 2) {
            fprintf(stderr, "usage: %s <host>\n", argv[0]);
            exit(0);
        }
        host = argv[1];
        /*
            Connecte au port par defaut du maitre 2121
        */
        client_maitrefd = Open_clientfd(host, port);
        req_maitre.type=NOUVELLE;
        rio_writen(client_maitrefd,&req_maitre,sizeof(req_maitre));
        //Recupere par le maitre les infos du serveur ou il doit se connecter
        rio_readn(client_maitrefd,&info_serveur,sizeof(info_serveur));
        clientfd=Open_clientfd(info_serveur.ip,info_serveur.port);
        close(client_maitrefd);
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
            req.type=type;
            //On verifie si le fichier n'est pas déja sur le pc et si il est on verifie qu'il n'a pas etait modifie par le serveur depuis
            snprintf(chemin_local,MAXLINE+256,"%s/%s",CLIENT_DIR,nom_fichier);
            if(stat(chemin_local,&st)==-1){
                req.octets_deja_recu=0;
                req.date_fichier=0;
            }
            else{
                req.octets_deja_recu=st.st_size;
                restant=-st.st_size;
                char chemin_info[MAXLINE];//Envoie la derniere date de modif de ce fichier par le serveur
                snprintf(chemin_info,MAXLINE+256,"%s/.%s.info",CLIENT_DIR,nom_fichier);
                int fd_info=open(chemin_info,O_RDONLY);
                if(fd_info!=-1){rio_readn(fd_info,&req.date_fichier,sizeof(time_t));}
                else {req.date_fichier=0;}
                close(fd_info);
            }
            strcpy(req.nom_ficher,nom_fichier);
            rio_writen(clientfd,&req,sizeof(req));
            rio_readn(clientfd,&rep,sizeof(rep));
            switch (rep.code_retour)
            {
            case FICHIER_NON_TROUVE:
                printf("Le fichier n'est pas dans le serveur\n");
                break;
            case ENVOIE_COMPLET :
            case ENVOIE_PARTIEL :
                //Garde en mémoire la date du fichier du serveur permet en cas de crash de verifier si le fichier source n'a pas changé
                char chemin_info[MAXLINE];
                snprintf(chemin_info,MAXLINE+256,"%s/.%s.info",CLIENT_DIR,nom_fichier);
                int fd_info=open(chemin_info,O_CREAT | O_WRONLY | O_TRUNC,0644);
                rio_writen(fd_info,&rep.date_modif,sizeof(time_t));
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
                int totale_recuperer=0;
                int memoire_octets_initiaux = req.octets_deja_recu;
                gettimeofday(&debut,NULL);//Lance le chrono
                while((restant>0))
                {   
                    if(restant>taille_bloc)
                        nb_recu = rio_readn(clientfd,buf,taille_bloc);
                    else{
                        nb_recu = rio_readn(clientfd,buf,restant);
                    }
                    if(nb_recu<=0){//Le serveur a crash on va en demander un nouveau au maitre
                        client_maitrefd = Open_clientfd(host, port);
                        req_maitre.type=PANNE;
                        req_maitre.port=info_serveur.port;
                        rio_writen(client_maitrefd,&req_maitre,sizeof(req_maitre));
                        //Recupere par le maitre les infos du nouveau serveur ou il doit se connecter
                        rio_readn(client_maitrefd,&info_serveur,sizeof(info_serveur));
                        clientfd=Open_clientfd(info_serveur.ip,info_serveur.port);
                        close(client_maitrefd);
                        //On refait une demande au nouveau serveur et on recupere ses information puis on relance telechargement
                        req.octets_deja_recu=memoire_octets_initiaux+totale_recuperer;//Nombre d'octets initaux puis ceux avant crash
                        rio_writen(clientfd,&req,sizeof(req));
                        rio_readn(clientfd,&rep,sizeof(rep));
                        //Pour etre sur de syncroniser le serveur et le client mais normalement inutile
                        restant=rep.taille_fichier;
                        continue;
                    }
                    //printf("Le client  a reçu %d bits\n",nb_recu);
                    rio_writen(fd_res,buf,nb_recu);
                    totale_recuperer+=nb_recu;
                    restant-=nb_recu;
                }
                gettimeofday(&fin,NULL);//Fin chrono
                temp_ecouler=(fin.tv_sec-debut.tv_sec)+(fin.tv_usec-debut.tv_usec)/1000000.0;
                printf("Fichier %s bien reçu, %d octets en %f secondes (%f Kbytes/s)\n",req.nom_ficher,totale_recuperer,temp_ecouler,(totale_recuperer/1024.0)/temp_ecouler);
                free(buf);
                close(fd_res);
                break;
            case DEJA_COMPLET:
                printf("Le fichier %s est déja complet et a jour sur le disque local\n",req.nom_ficher);
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
