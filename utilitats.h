#ifndef UTILITATS_H
#define UTILITATS_H


typedef struct {
    char** lista;//lista de strings cada string maxim 256 bytes
    int qt;
    int dim;
} t_listaCadDin;


char** inicializarLista();
int ampliarLista(t_listaCadDin *d_lista);
int limpiarLista(t_listaCadDin *d_lista);



#endif