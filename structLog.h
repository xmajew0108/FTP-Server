#ifndef STRUCTLOG_H
#define STRUCTLOG_H

typedef struct{
    char user[64];
    char passHash[65];
    char ip[16];
}t_log;

#endif