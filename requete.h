typedef enum typereq_t {GET,PUT,LS} typereq_t;

typedef struct request_t
{
    int type;
    char * nom_ficher;

} request_t;