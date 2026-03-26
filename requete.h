#include "csapp.h"
typedef enum typereq_t {GET,PUT,LS} typereq_t;

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
