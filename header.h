#ifndef HEADER_H
#define HEADER_H


// puerto
#define PORT 10001

//status de respuesta
#define OK 0
#define ERROR 1


// operaciones
#define OP_DIR 1
#define OP_CD 2
#define OP_GET 3
#define OP_RGET 4
#define OP_QUIT 5

// longitud de un paquete
#define LONG_PACK 2048


typedef struct {
    char user[64];
    char passHash[65];  // SHA256 hex = 64 chars + \0
} t_loginReq;

typedef struct{// header consta de operacion(ope), version(vers) y longitud(len)
    int ope;
    int vers;
    int len;
}t_reqHead; 

typedef struct{ // header de respuesta, status(status), numero de paquetes(qtPacks) y mensaje(info)
    int status;
    int qtPacks;
    char info[256];
}t_respHead;







#endif