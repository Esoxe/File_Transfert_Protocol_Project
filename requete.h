#include "csapp.h"
#define TAILLE_BLOC 8192

typedef enum typereq_t {GET,PUT,LS,BYE} typereq_t;

typedef struct request_t
{
    int type;
    char nom_ficher[MAXLINE];

} request_t;

typedef struct  response_t
{
    int taille_fichier;
    int code_retour;
}response_t;
