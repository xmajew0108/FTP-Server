#define _DEFAULT_SOURCE  
#define _BSD_SOURCE      
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h> 
#include <sys/wait.h>
#include <signal.h>
#include <netinet/in.h>         
#include <arpa/inet.h>          
#include <string.h>             
#include <unistd.h>             
#include <fcntl.h>
#include "header.h"
#include "utilitats.h"
#include "structLog.h"

int cli; // fd de cliente
int serv; // fd de servidor
int fdRoot;

#define INIT_PATH "/home/xuban-psp/Documentos/FTP/"

#define SEND_RESP(cli, resp, st, pack, msg)               \
    do {                                                       \
        (resp).status  = (st);                             \
        (resp).qtPacks = (pack);                               \
        strncpy((resp).info, (msg), strlen(msg)+1);             \
        (resp).info[strlen(msg)] = '\0';                       \
        send((cli), &(resp), sizeof(resp), 0);                 \
    } while(0)                              



void handleSignal(int sig) {         
    close(fdRoot);               
    
    if (cli > 0) close(cli);
    exit(0);
}

void handleSignalFather(int sig) {   
    signal(SIGTERM, SIG_IGN);        
    kill(0, SIGTERM);
    close(fdRoot);                
    while (waitpid(-1, NULL, 0) > 0); 
    if (cli  > 0) close(cli);
    if (serv > 0) close(serv);
    exit(0);
}




bool handleLogin(struct sockaddr_in *info, int client) {
    t_loginReq req={0};
    t_log log={0};
    t_log readLogs={0};
    t_respHead resp={0};
    int fdlog=0;

    recv(client,&req,sizeof(req),0);
    strncpy(log.user,req.user,sizeof(log.user));
    strncpy(log.passHash,req.passHash,sizeof(log.passHash));
    strncpy(log.ip,inet_ntoa(info->sin_addr),sizeof(log.ip));

    if ((fdlog=open("log.txt",O_RDWR|O_CREAT|O_APPEND,0644))<0){
        perror("open");
        return false;
    }

    int bytes=0;
    while((bytes=read(fdlog,&readLogs,sizeof(readLogs)))>0){
        if (strcmp(readLogs.user,log.user)==0 && strcmp(readLogs.passHash,log.passHash)==0){
            if (strcmp(readLogs.ip,log.ip)==0){
                printf("found\n");
                return true;
            }else{
                printf("IP incorrecta\n");
                return false;
            }
        }
    }
    printf("no encontrado\n");
    printf("logeando...\n");
    lseek(fdlog,0,SEEK_END);
    write(fdlog,&log,sizeof(log));
    close(fdlog);
    return true;



}


int lecturaFichero(char *file,t_respHead *resp,bool isDir){
    int fd=0;
    char buff[LONG_PACK]={0};
    int bytes=0;

    if (access(file,F_OK)<0){
        SEND_RESP(cli,*resp,ERROR,0,"error al acceder al archivo");
        return -1;
    }

    if ((fd=open(file,O_RDONLY))<0){
        SEND_RESP(cli,*resp,ERROR,0,"error al abrir el archivo");
        return -1;
    }
    
    SEND_RESP(cli,*resp,OK,0,"archivo leido correctamente");
    
    recv(cli,resp,sizeof(t_respHead),0);
    
    if (resp->status==ERROR){
        close(fd);
        return -1;
    }

    while((bytes=read(fd,buff,LONG_PACK-1))>0){
        if (isDir){
            buff[bytes]='\0';
        }
        printf("bytes: %d\n",bytes);
        send(cli,buff,bytes,0);
    }

    buff[0]='\r';
    buff[1]='\n';
    buff[2]='\0';
    send(cli,buff,LONG_PACK,0);
    close(fd);

    if (bytes<0){
        SEND_RESP(cli,*resp,ERROR,0,"error al leer el archivo");
        return -1;
    }else{
        SEND_RESP(cli,*resp,OK,0,"archivo leido correctamente");
    }

    return 0;
}
 

int dir(int cli){
    
    t_listaCadDin defi={0};
    struct stat files;
    struct dirent *de;
    t_respHead resp={0};

    defi.dim=10;
    defi.qt=0;
    defi.lista=inicializarLista();


    if (defi.lista==NULL){
        SEND_RESP(cli,resp,ERROR,0,"error al inicializar lista");
        return 1;
    }

    DIR *dr = opendir(".");
    if (dr==NULL){
        SEND_RESP(cli,resp,ERROR,0,"error al abrir directorio");
        return -1;
    }
    SEND_RESP(cli,resp,OK,0,"directorio abierto correctamente");
    
    while((de = readdir(dr)) != NULL){
        if (de->d_name[0]!='.'){
            if (defi.qt>=defi.dim){
                if (ampliarLista(&defi)<0){
                    SEND_RESP(cli,resp,ERROR,0,"error al ampliar lista");
                    closedir(dr);
                    limpiarLista(&defi);
                    return -1;
                }
            }
            if (de->d_type==DT_DIR){
                sprintf(defi.lista[defi.qt],"%s <DIR>\n",de->d_name);
            }else{
                stat(de->d_name,&files);
                sprintf(defi.lista[defi.qt],"%s - %lld bytes\n", de->d_name,files.st_size);
            }
            defi.qt++;
        }
    }
    closedir(dr);
    SEND_RESP(cli,resp,OK,defi.qt,"Enviando datos...");
    for(int i=0;i<defi.qt;i++){
        send(cli,defi.lista[i],256,0);
    }
    limpiarLista(&defi);
    return 0;
}

int cd(int cli, int size){

    t_respHead resp={0};
    char *dir=(char *)malloc(size+1);


    recv(cli,dir,size,0);
    dir[size]='\0';
    printf("dir: %s\n",dir);
    if (chdir(dir)<0){
        SEND_RESP(cli,resp,ERROR,0,"error al cambiar de directorio");
        free(dir);
        return -1;
    }
    free(dir);
    char cwd[256];
    getcwd(cwd,256);
    printf("cwd: %s\n",cwd);
    SEND_RESP(cli,resp,OK,strlen(cwd),"directorio cambiado correctamente");
    
    return 0;
}

int get(int cli,int size){

    t_respHead resp={0};
    char *file = (char *)malloc(size+1);

    recv(cli,file,size,0);
    file[size]='\0';
    printf("file: %s\n",file);

    lecturaFichero(file,&resp,false);
    free(file);

    return 0;
}

int rget(int cli,int size){
    t_respHead resp={0};
    char *file = (char *)malloc(size+1);

    recv(cli,file,size,0);
    file[size]='\0';
    printf("file: %s\n",file);
    int pid=fork();
    if (pid<0){
        perror("fork");
        return -1;
    }


    if (pid == 0) {
        close(cli);
        close(serv);
    struct stat st;
    if (stat(file, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            char cwd[256];
            getcwd(cwd, 256);
            printf("cwd: %s\n", cwd);
            fchdir(fdRoot);
            chroot(".");
            chdir(INIT_PATH);   // opcional pero recomendable
            getcwd(cwd, 256);
            printf("cwd: %s\n", cwd);
            chdir(cwd);
            getcwd(cwd, 256);
            printf("cwd: %s\n", cwd);
            printf("Comprimiendo: %s\n", file);

            execl("/bin/tar", "tar", "-czf", "fichero.tar.gz", file, NULL);

            perror("execl");
            exit(1);
        }
        exit(2);
    }
    exit(3);
}
    int status;
    waitpid(pid,&status,0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        SEND_RESP(cli,resp,OK,0,"Archivo comprimido correctamente");
        lecturaFichero("fichero.tar.gz", &resp,true);      
        unlink("fichero.tar.gz");
    }else{
        SEND_RESP(cli,resp,ERROR,0,"error al ejecutar tar");
    }
    free(file);

    return 0;
}

void handleClient(int cli){
    t_reqHead op={0};
    t_respHead resp={0};
    int qt_err=0;   
    while(op.ope!=OP_QUIT){
        int n=recv(cli,&op,sizeof(op),0);
        if (n==0){
            op.ope=OP_QUIT;
        }
        if (n<0){
            printf("error recibiendo paquete\n");
            qt_err++;
            if (qt_err>10){
                op.ope=OP_QUIT;
            }
            continue;
        }

        switch(op.ope){
            case OP_DIR:
                dir(cli);
                break;
            case OP_CD:
                cd(cli,op.len);
                break;
            case OP_GET:
                get(cli,op.len);
                break;
            case OP_RGET:
                rget(cli,op.len);
                break;
            case OP_QUIT:
                close(cli);
                cli=0;
                close(fdRoot);
                printf("client disconnected\n");
                break;
            default:
                SEND_RESP(cli,resp,ERROR,0,"error en operacion");
                break;
        }


    }
    return;

}


int main(int argc,char** argv){
    fdRoot=open("/",O_RDONLY);
    signal(SIGINT,  handleSignalFather);
    signal(SIGTERM, handleSignalFather);
    signal(SIGPIPE, SIG_IGN);


    cli=serv=0; 
    struct sockaddr_in t_serv,t_cli; // struct para sockets de cliente y servidor

    if ((serv=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
        perror ("socket");
        return 1;
    }

    t_serv.sin_family=AF_INET;
    t_serv.sin_port=htons(PORT);
    t_serv.sin_addr.s_addr=INADDR_ANY;
    
    for (int i=0;i<8;i++){
        t_serv.sin_zero[i]=0;
    }


    if (bind(serv,(struct sockaddr*)&t_serv,sizeof(t_serv))<0){
        perror("bind");
        return 1;
    }

    if (listen(serv,10)<0){
        perror("listen");
        return 1;
    }


    socklen_t len=sizeof(t_cli);
    int conexions=1;
    while(conexions!=0){

        

        t_respHead resp;

        printf("waiting for connection...\n");

        if ((cli=accept(serv,(struct sockaddr*)&t_cli,&len))<0){
            perror("accept");
            return 1;
        }

        printf("client connected\n");
        int pid=fork();

        if (pid<0){
            perror("fork");
            SEND_RESP(cli,resp,ERROR,0,"error al recibir conexion");
            return 1;
        }

        if (pid==0){
            
            signal(SIGINT,  handleSignal);  
            signal(SIGTERM, handleSignal);
            close(serv);
            
            bool conectado=handleLogin(&t_cli,cli);
            
            int x=chroot(INIT_PATH);


            if (x<0){
                perror("asignacion de path");
                SEND_RESP(cli,resp,ERROR,0,"asignacion de path");
                return 1;
            }

            if (chdir("/") < 0) {
                perror("error cambiando de directorio tras chroot");
                SEND_RESP(cli,resp,ERROR,0,"error cambiando de directorio tras chroot");
                return 1;
            }

            
            if (conectado){
                SEND_RESP(cli,resp,OK,0,"login correcto");
                handleClient(cli);
            }else{
                SEND_RESP(cli,resp,ERROR,0,"login incorrecto");
            }

            cli=0;
            return 0;
        }
        close(cli);
        conexions++;

    }

    close(serv);
    return 0;
}