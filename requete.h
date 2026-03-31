#include "csapp.h"

typedef enum typereq_t {GET,RM,PUT,LS,BYE} typereq_t;
//Indique au maitre si il etais deja connecter a un autre serveur et le serveur a crash ou si nouvelle connexion
typedef enum typereq_maitre_t {NOUVELLE,PANNE} typereq_maitre_t;


typedef struct request_t
{
    int type;
    char nom_ficher[MAXLINE];
    int taille_fichier;
    size_t octets_deja_recu;
    time_t date_fichier;

} request_t;

typedef struct 
{
    typereq_maitre_t type;
    int port;
}requete_maitre_t;


//Structure donnant le serveur ou doit se connecter le client
typedef struct 
{
    char ip[INET_ADDRSTRLEN];
    int port;
}response_maitre_t;

//Structure permettant d'initier le transfert entre le serveur et le client 
typedef struct  response_t
{
    int taille_bloc;
    int taille_fichier;
    int code_retour;
    time_t date_modif; //Permet de sauvegarder la date de derniere modif du fichier dans le serveur

}response_t;

//Liste des codes de retour
#define SUCCES 0
#define ECHEC 1
#define FICHIER_NON_TROUVE 404
#define FIN_CONNEXION 67
#define ENVOIE_COMPLET 100
#define ENVOIE_PARTIEL 104
#define DEJA_COMPLET 300
#define ENVOIE_LS 236
#define ERREUR_LS 237
#define READY_PUT 333
