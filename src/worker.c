#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "../include/queue.h"
#include "../include/aux.h"

#define UNIX_PATH_MAX 108
#define SOCKNAME "./farm.sck"

#define MAX_LEN_PATH 256

/////////////////////////////////////////////////

long result(char* name){//calcola risultato
    long s=0, l, i=0;
    FILE *f;
    char* tobeused = name;
    
    if((f = fopen(tobeused, "rb")) == NULL){
        perror("WORKER fopen - ");
        exit(-1);
    }
    
    while(fread(&l, sizeof(long), 1, f) == 1){
        s = s + (i*l);
        i++;
    }
    
    fclose(f);
    
    return s;
}

int send_res(res result){

    //connect
    int fd_skt;
    struct sockaddr_un sa;
    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd_skt == -1){
        perror("worker socket - ");
        return -1;
    }

    while(connect(fd_skt, (struct sockaddr*)&sa, sizeof(sa)) == -1){
        if(errno == ENOENT || errno == ECONNREFUSED)
            sleep(1);
        else{
            perror("WORKER - connect");
            return -1;
        }
    }

    int left, r;
    char* tobesent;

    //send result (long)
    left = sizeof(result.r);
    tobesent = (char*)&result.r;
    while(left > 0){
        if((r = write(fd_skt, tobesent, left)) == -1){
            perror("worker write 1 - ");
            return -1;
        }

        left = left - r;
        tobesent = tobesent + r;
    }

    //send str length
    left = sizeof(int);
    int len = strlen(result.file_name);
    tobesent = (char*)&len;
    while(left > 0){
        if((r = write(fd_skt, tobesent, left)) == -1){
            perror("worker write 2 - ");
            return -1;
        }

        left = left - r;
        tobesent = tobesent + r;
    }

    //send filename
    left = strlen(result.file_name);
    tobesent = result.file_name;
    while(left > 0){
        if((r = write(fd_skt, tobesent, left)) == -1){
            perror("worker write 3 - ");
            return -1;
        }

        left = left - r;
        tobesent = tobesent + r;
    }

    if(close(fd_skt) == -1){
        perror("worker close - ");
        return -1;
    }

    return 0;
}

/////////////////////////////////////////////////

void* worker(void * Q){
    int worker_done = 0;

    //printf("[WORKER] %d hello\n", gettid());

    // //connect
    // int fd_skt;
    // struct sockaddr_un sa;
    // strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    // sa.sun_family = AF_UNIX;
    // fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);

    // while(connect(fd_skt, (struct sockaddr*)&sa, sizeof(sa)) == -1){
    //     if(errno == ENOENT)
    //         sleep(1);
    //     else{
    //         printf("[WORKER] connection error\n");
    //         //return -1;
    //         //exit(-1);
    //         return NULL;
    //     }
    // }

    //loop
    res r;
    while(!worker_done){
        if(dequeue((queue*)Q, r.file_name) == 0){
            //check if done
            if(strcmp(r.file_name, "DONE") == 0){
                worker_done = 1;
            }

            //check if -thread
            else if(strcmp(r.file_name, "DELETE") == 0){
                 worker_done = 1;
                 //printf("[WORKER] %d received DELETE\n", gettid());
            }

            //else
            else{
                r.r = result(r.file_name);
                send_res(r);
            }
        }
    }

    //exit seq
    //close(fd_skt);

    //printf("[WORKER] %d bye\n", gettid());

    return NULL;
}