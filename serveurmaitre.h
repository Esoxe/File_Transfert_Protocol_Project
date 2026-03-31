#include "csapp.h"
#include "requete.h"

#define NB_SLAVES 3
#define PORT_MAITRE 2121


typedef struct {
    char ip[INET_ADDRSTRLEN];
    int port;
} info_esclave_t;

//Annuaire des différents IP et adresses des serveurs
//Les ports doivents différent pour que le code fonctionne même si théoriquement pas nécessaire si différentes machines
//Car les ports sont utilisé comme identifiants
static info_esclave_t CONFIG_SERVEURS[NB_SLAVES]= {
    {"127.0.0.1",2207},
    {"127.0.0.1",2208},
    {"127.0.0.1",2209}
};