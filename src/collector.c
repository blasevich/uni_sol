#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <pthread.h>

#include "../include/aux.h"

#define MAX_LEN_PATH 256

#define UNIX_PATH_MAX 108
#define SOCKNAME "./farm.sck"

static res * R = NULL;
static int dimR = 0;

static int collector_done = 0;

///////////////////////////////////////////////////////

int compare(const void *a, const void *b){ //aux qsort
    res * A = (res *)a;
    res * B = (res *)b;

    if(A->r < B->r)
        return -1;
    else if(A->r > B->r)
        return 1;
    else
        return 0;
}

void sort(res* V, int Size){//sort results
    qsort(V, Size, sizeof(res), compare);
}

void print(res* v, int Size){ //print results
    int i;

    sort(v, Size);
    
    for(i=0; i<Size; i++){
        printf("%ld %s\n", v[i].r, v[i].file_name);
    }
}

///////////////////////////////////////////////////////

void* handleresult(){
    do{
        if(sleep(1) == -1){
            printf("[COLLECTOR] sleep\n");
        }

        if(R == NULL){
            printf("[COLLECTOR] handle result - no results\n");
            return NULL;
        }

        //sort(R, dimR);
        print(R, dimR);

    }while(!collector_done);

    return NULL;

}

int server(){
    
    int fd_skt, fd_c; //fd_c == connection socket
    struct sockaddr_un sa;
    res * newR;

    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;

    fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd_skt == -1){
        perror("collector socket - ");
        return -1;
    }
    if(bind(fd_skt, (struct sockaddr*)&sa, sizeof(sa)) == -1){
        perror("collector bind - ");
        return -1;
    }
    if(listen(fd_skt, SOMAXCONN) == -1){
        perror("collector listen - ");
        return -1;
    }

    int str_len;
    res new;
    ssize_t bytesread, toread;

    while(!collector_done){
        fd_c = accept(fd_skt, NULL, 0);
        if(fd_c == 1){
            perror("collector accept - ");
        }

        //read result
        bytesread = 0;
        toread = sizeof(long);
        while(toread > 0){
            if((bytesread = read(fd_c, &new.r+bytesread, toread)) == -1){
                perror("collector read 1 - ");
                return -1;
            }
            toread = toread - bytesread;
        }

        // read string length
        bytesread = 0;
        toread = sizeof(int);
        while(toread > 0){
            if((bytesread = read(fd_c, &str_len+bytesread, toread)) == -1){
                perror("collector read 2 - ");
                return -1;
            }
            toread = toread - bytesread;
        }

        //read filename
        bytesread = 0;
        toread = str_len;
        while(toread > 0){
            if((bytesread = read(fd_c, new.file_name+bytesread, str_len)) == -1){
                perror("collector read 3 - ");
                return -1;
            }
            toread = toread - bytesread;
        }

        new.file_name[str_len] = '\0';

        if(strcmp(new.file_name, "DONE") != 0){
            //add new result to queue
            dimR++;

            newR = realloc(R, dimR*sizeof(res));
            if(newR == NULL){
                perror("COLLECTOR realloc - ");
                collector_done = 1;
            }
            R = newR;
            strcpy(R[dimR-1].file_name, new.file_name);
            R[dimR-1].r = new.r;

        }else{
            collector_done = 1;
        }

        if(close(fd_c) == -1){
            perror("collector close 1 - ");
        }
        
    }

    if(close(fd_skt) == -1){
        perror("collector close 2 - ");
    }

    return 0;
}

///////////////////////////////////////////////////////

void collector(){

    //signals
    struct sigaction s;
    memset(&s, 0, sizeof(s));
    s.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &s, NULL);
    sigaction(SIGHUP, &s, NULL);
    sigaction(SIGINT, &s, NULL);
    sigaction(SIGQUIT, &s, NULL);
    sigaction(SIGTERM, &s, NULL);
    sigaction(SIGUSR1, &s, NULL);
    sigaction(SIGUSR2, &s, NULL);

    R = malloc(sizeof(res));

    pthread_t pt;
    int e;

    if((e = pthread_create(&pt, NULL, &handleresult, (void*)R))){
        fprintf(stderr, "[COLLECTOR] pthread_create - %d\n", e);
        exit(-1);
    }

    server();

    //wait for thread
    if((e = pthread_join(pt, NULL))){
        fprintf(stderr, "[COLLECTOR] pthread_join - %d\n", e);
        exit(-1);
    }

    //handleresult();

    free(R);

}