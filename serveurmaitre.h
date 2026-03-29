#include "csapp.h"
#include "requete.h"

#define PORT_DEBUT_ESCLAVE 2207
#define NB_SLAVES 3
#define PORT_MAITRE 2121


typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} info_esclave_t;
