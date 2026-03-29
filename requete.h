#include "csapp.h"

typedef enum typereq_t {GET,PUT,LS,BYE} typereq_t;


typedef struct request_t
{
    int type;
    char nom_ficher[MAXLINE];
    size_t octets_deja_recu;
    time_t date_fichier;

} request_t;

typedef struct  response_t
{
    int taille_bloc;
    int taille_fichier;
    int code_retour;
    time_t date_modif; //Permet de sauvegarder la date de derniere modif du fichier dans le serveur

}response_t;

//Liste de codes de retour
#define SUCCES 0
#define FICHIER_NON_TROUVE 404
#define FIN_CONNEXION 67
#define ENVOIE_COMPLET 100
#define ENVOIE_PARTIEL 104
