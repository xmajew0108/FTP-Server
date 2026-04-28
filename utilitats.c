#include "utilitats.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

char** inicializarLista(){
    char** lista;
    lista =(char**) calloc(10,sizeof(char*));

    if (lista==NULL){
        perror("calloc");
        return NULL;
    }
    
    for (int i=0;i<10;i++)
        lista[i]=NULL;

    for(int i=0;i<10;i++){
        lista[i]=calloc(256,sizeof(char));
        if (lista[i]==NULL){
            for (int j=0;j<i;j++){
                free(lista[j]);
            }
            free(lista);
            perror("calloc");
            return NULL;
        }
    }
    return lista;
}

int ampliarLista(t_listaCadDin *d_lista){
    d_lista->dim+=10;
    char** temp=realloc(d_lista->lista,sizeof(char*)*d_lista->dim);
    if (temp==NULL){
        perror("realloc");
        return -1;
    }
    for (int i=d_lista->qt;i<d_lista->dim;i++){
        temp[i]=calloc(256,sizeof(char));
        if (temp[i]==NULL){
            perror("calloc");
            return -1;
        }
    }
    d_lista->lista=temp;
    return 0;
}


int limpiarLista(t_listaCadDin *d_lista){
    for (int i=0;i<d_lista->dim;i++){
        if (d_lista->lista[i]!=NULL)
            free(d_lista->lista[i]);
    }
    free(d_lista->lista);
    d_lista->lista=NULL;
    d_lista->dim=0;
    d_lista->qt=0;
    return 0;
}
