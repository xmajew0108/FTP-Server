#define _DEFAULT_SOURCE  // For modern glibc
#define _BSD_SOURCE      // For older glibc
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <signal.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>         // socket(), bind(), listen(), accept(), connect()
#include <netinet/in.h>         // struct sockaddr_in
#include <arpa/inet.h>          // inet_addr()
#include <string.h>             // strcmp()
#include <unistd.h>             // recv(), send(), close()
#include <fcntl.h>
//#include <openssl/evp.h>
#include "header.h"



#define SEND_REQ(cli,req,doOpe,lon,ver)               \
    do {                                                       \
        (req).ope  = (doOpe);                             \
        (req).vers = (ver);                               \
        (req).len  = (lon);                               \
        send((cli), &(req), sizeof(req), 0);                 \
    } while(0)

#define SEND_RESP(cli, resp, st, pack, msg)               \
    do {                                                       \
        (resp).status  = (st);                             \
        (resp).qtPacks = (pack);                               \
        strncpy((resp).info, (msg), strlen(msg)+1);             \
        (resp).info[strlen(msg)] = '\0';                       \
        send((cli), &(resp), sizeof(resp), 0);                 \
    } while(0)    

int conn;

static void sha256hex(const char *input, char *output) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char *)input, strlen(input), hash);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(output + (i * 2), "%02x", hash[i]);
    output[64] = '\0';
}

bool login(){
    t_loginReq req  = {0};
    t_respHead resp = {0};
    char pass[128]  = {0};

    printf("Usuario: ");
    scanf("%63s", req.user);

    printf("Contraseña: ");
    scanf("%127s", pass);

    sha256hex(pass, req.passHash);
    memset(pass, 0, sizeof(pass));  

    printf("Hash: %s\n", req.passHash);  

    send(conn, &req, sizeof(req), 0);

    printf("resp.info: %s\n", resp.info);

    return (resp.status == OK) ? true : false;

}

void handleSignal(int sig){
    close(conn);
    exit(0);
}

int dir(){
    t_reqHead ord={0};
    SEND_REQ(conn,ord,OP_DIR,0,0);


    t_respHead resp={0};
    recv(conn,&resp,sizeof(t_respHead),0);
    #ifdef DEBUG
        printf("abriendo dir\n");
        printf("resp.status: %d\n",resp.status);
        printf("resp.qtPacks: %d\n",resp.qtPacks);
        printf("resp.info: %s\n",resp.info);
    #endif
    if (resp.status==ERROR){
        return -1;
    }
    recv(conn,&resp,sizeof(t_respHead),0);
    #ifdef DEBUG
        printf("leyendo dir\n");
        printf("resp.status: %d\n",resp.status);
        printf("resp.qtPacks: %d\n",resp.qtPacks);
        printf("resp.info: %s\n",resp.info);
    #endif

    char* dir =(char*) calloc(256,sizeof(char));

    for (int i=0;i<resp.qtPacks;i++){
        recv(conn,dir,256,0);
        printf("%d==> %s\n",i,dir);
    }
    free(dir);
    return 0;

}

int cd(){
    printf("cd: ");
    t_reqHead req={0};
    char dir[256]={0};
    scanf("%s",dir);
    SEND_REQ(conn,req,OP_CD,strlen(dir)+1,0);
    send(conn,dir,strlen(dir)+1,0);
    t_respHead resp={0};
    recv(conn,&resp,sizeof(t_respHead),0);
    #ifdef DEBUG
        printf("abriendo dir\n");
        printf("resp.status: %d\n",resp.status);
        printf("resp.qtPacks: %d\n",resp.qtPacks);
        printf("resp.info: %s\n",resp.info);
    #endif
    if (resp.status==ERROR){
        return -1;
    }
    return 0;
    
}


int get(){

    char buff[LONG_PACK]={0};
    char file[256]={0};
    t_reqHead req={0};
    t_respHead resp={0};
    int fd=0;

    printf("get: ");
    scanf("%s",file);

    SEND_REQ(conn,req,OP_GET,strlen(file)+1,0);
    send(conn,file,strlen(file)+1,0);
    recv(conn,&resp,sizeof(t_respHead),0);
    #ifdef DEBUG
        printf("abriendo get\n");
        printf("resp.status: %d\n", resp.status);
        printf("resp.qtPacks: %d\n", resp.qtPacks);
        printf("resp.info: %s\n", resp.info);
    #endif
    if (resp.status==ERROR){
        return -1;
    }

    if((fd=open(file,O_WRONLY|O_CREAT|O_TRUNC,0644))<0){
        perror("open");
        SEND_RESP(conn,resp,ERROR,0,"error al abrir el archivo");
        return -1;
    }else{
        SEND_RESP(conn,resp,OK,0,"archivo abierto correctamente");
    }

    int bytes=0;
    while(1){
        bytes=recv(conn,buff,LONG_PACK,0);
        if (buff[0]=='\r' && buff[1]=='\n'){
            break;
        }
        if (bytes<=0){
            break;
        }
        write(fd,buff,strlen(buff));
    }
    close(fd);
    recv(conn,&resp,sizeof(t_respHead),0);
    if (resp.status==ERROR){
        unlink(file);
        return -1;
    }

    close(fd);
    return 0;
}


int rget(){
    
    t_reqHead req={0};
    t_respHead resp={0};
    char buff[256]={0};
    char file[LONG_PACK]={0};
    int fd=0;

    printf("rget: ");
    scanf("%s",buff);
    SEND_REQ(conn,req,OP_RGET,strlen(buff)+1,0);
    send(conn,buff,strlen(buff)+1,0);
    recv(conn,&resp,sizeof(t_respHead),0);
    #ifdef DEBUG
        printf("abriendo rget\n");
        printf("resp.status: %d\n",resp.status);
        printf("resp.qtPacks: %d\n",resp.qtPacks);
        printf("resp.info: %s\n",resp.info);
    #endif
    if (resp.status==ERROR){
        return -1;
    }

    recv(conn,&resp,sizeof(t_respHead),0);
    #ifdef DEBUG
        printf("leyendo rget\n");
        printf("resp.status: %d\n",resp.status);
        printf("resp.qtPacks: %d\n",resp.qtPacks);
        printf("resp.info: %s\n",resp.info);
    #endif

    if (resp.status==ERROR){
        return -1;
    }
    if ((fd=open("fichero.tar.gz",O_WRONLY|O_CREAT|O_TRUNC,0644))<0){
        perror("open");
        SEND_RESP(conn,resp,ERROR,0,"error al abrir el archivo");
        return -1;
    }
    SEND_RESP(conn,resp,OK,0,"archivo abierto correctamente");

    int bytes=0;
    while(1){
        bytes=recv(conn,file,LONG_PACK,0);
        if (file[0]=='\r' && file[1]=='\n'){
            break;
        }
        if (bytes<=0){
            break;
        }
        write(fd,file,bytes);
    }

    recv(conn,&resp,sizeof(t_respHead),0);

    #ifdef DEBUG
        printf("cerrando rget\n");
        printf("resp.status: %d\n",resp.status);
        printf("resp.qtPacks: %d\n",resp.qtPacks);
        printf("resp.info: %s\n",resp.info);
    #endif

    if (resp.status==ERROR){
        unlink("fichero.tar.gz");
        return -1;
    }
    close(fd);

    int pid=fork();
    if (pid<0){
        perror("fork");
        return -1;
    }
    if (pid==0){
        execlp("tar","tar","-xzf","fichero.tar.gz",NULL);
        perror("execlp");
        exit(1);
    }
    waitpid(pid,NULL,0);
    unlink("fichero.tar.gz");
    close(conn);


    return 0;

    
}

int main(int argc, char** argv){
    int res;
    struct sockaddr_in cli;
    signal(SIGINT,handleSignal);
    if ((conn=socket(AF_INET,SOCK_STREAM,0))<0){
        perror("socket");
        return 1;
    }

    cli.sin_family=AF_INET;
    cli.sin_addr.s_addr=inet_addr("10.150.2.140");
    cli.sin_port=htons(PORT);

    for(int i =0;i<8;i++){
        cli.sin_zero[i]=(unsigned char)0;
    }

    if ((res=connect(conn,(struct sockaddr*)&cli,sizeof(cli)))<0){
        perror("connect");
        close(conn);
        return 1;
    }
    t_respHead resp={0};
    t_reqHead req={0};
    int salida=1;
    // recv(conn,&resp,sizeof(t_respHead),0);
    // #ifdef DEBUG
    //     printf("resp.status: %d\n",resp.status);
    //     printf("resp.qtPacks: %d\n",resp.qtPacks);
    //     printf("resp.info: %s\n",resp.info);
    // #endif

    bool log= login();

    if (!log){
        close(conn);
        return -1;
    }

    recv(conn,&resp,sizeof(t_respHead),0);

    #ifdef DEBUG
        printf("resp.status: %d\n",resp.status);
        printf("resp.qtPacks: %d\n",resp.qtPacks);
        printf("resp.info: %s\n",resp.info);
    #endif
    if (resp.status==ERROR){
        close(conn);
        return -1;
    }

    while(salida>0 || salida<0){
        
        if(resp.status==ERROR){
            salida=0;
            continue;
        }
        printf("========MENU========\n");
        printf("\t1. DIR\n");
        printf("\t2. CD\n");
        printf("\t3. GET\n");
        printf("\t4. RGET\n");
        printf("\t5. QUIT\n");
        

        int opcion=0;
        int lec=0;
        do{
            lec=scanf("%d",&opcion);
            if (lec!=1){
                printf("error al leer opcion\n");
                char c;
                while((c=getchar())!='\n' && c!=EOF);
            }
        }while(lec!=1);

        switch(opcion){
            case OP_DIR:
                dir();
                break;
            case OP_CD:
                cd();
                break;
            case OP_GET:
                get();
                break;
            case OP_RGET:
                rget();
                break;
            case OP_QUIT:
                salida=0;
                SEND_REQ(conn,req,OP_QUIT,0,0);
                conn=0;
                break;

        }

    }
    return 0;
}