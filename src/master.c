#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/un.h>

#include "../include/aux.h"
#include "../include/queue.h"

int master_done = 0;

static int thread_incr = 0;
static int thread_decr = 0;

static pthread_t *thread_pool, *new_thread_pool;
static int dim; //nthreads
static queue* Q;

void* worker(void*);

//////////////////////////////////////////////

void* sighandler(void * arg){
    sigset_t *set = (sigset_t*)arg;
    int e;

    while(!master_done){
        int s;
        if(sigwait(set, &s)){
            perror("sigwait failed");
            return NULL;
        }

        switch(s){
            case SIGHUP:
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
                master_done = 1;
                break;
            case SIGUSR1:
                thread_incr = 1;
                break;
            case SIGUSR2:
                thread_decr = 1;
                break;
            default:
                ;
        }

        if(thread_incr){
            thread_incr = 0;

            new_thread_pool = realloc(thread_pool, (dim+1)*sizeof(pthread_t));
            if(new_thread_pool == NULL){
                perror("MASTER realloc - ");
            }
            else{
                dim++;
                thread_pool = new_thread_pool;
                if((e = pthread_create(&thread_pool[dim-1], NULL, &worker, (void*)Q))){
                    fprintf(stderr, "[MASTER] pthread_create - %d\n", e);
                    exit(-1);
                }
            }

        }
        if(thread_decr){
            thread_decr = 0;

            if(dim > 1){
                
                enqueue(Q, "DELETE");
                dim--;
                
            }
            else{
                //printf("[MASTER] minimum number of threads already reached\n");
            }
        }
    }

    return NULL;
}

//////////////////////////////////////////////

void master(int nthreads, int qlen, int delay, char * dir, char **files, int num_files){

    if(dir == NULL && files == NULL){
        printf("[MASTER] no files\n");
        return;
    }

    if(nthreads <= 0){
        printf("[MASTER] min nthreads: 1\n");
        return;
    }
    else{
        dim = nthreads;
    }

    //handle signals
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGCONT);
    if(pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0){ //R
        fprintf(stderr, "pthread_sigmask\n");
        abort();
    }

    struct sigaction s_ign;
    memset(&s_ign, 0, sizeof(s_ign));
    s_ign.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &s_ign, NULL); //ignore

    //sighandler thread
    pthread_t sighandler_thread;
    int e;

    if((e = pthread_create(&sighandler_thread, NULL, sighandler, &mask))){
        fprintf(stderr, "[MASTER] pthread_create - %d\n", e);
        exit(-1);
    }

    char* worker_file = "nworkeratexit.txt";

    Q = (queue *)malloc(sizeof(queue));
    if(Q == NULL){
        perror("MASTER malloc 1 - ");
        return;
    }

    init(Q, qlen);

    thread_pool = malloc(dim*sizeof(pthread_t));
    if(thread_pool == NULL){
        perror("MASTER malloc 2 - ");
        del(Q);
        free(Q);
        return;
    }

    for(int i=0; i<dim; i++){
        if((e = pthread_create(&thread_pool[i], NULL, &worker, (void*)Q))){
            fprintf(stderr, "[MASTER] pthread_create %d - %d\n", i, e);
            exit(-1);
        }
    }

    //handle files
    if(files != NULL && !master_done){
        int _isreg;
        for(int i=0; i<num_files; i++){
            _isreg = is_reg(files[i]);
            if(_isreg && !master_done){
                
                //add file to queue
                enqueue(Q, files[i]);
                //free(files[i]);
                
                if(usleep(delay) == -1){
                    perror("master usleep - ");
                }
            }
            if(!_isreg){
                //free(files[i]);
            }
        }

        //free(files);
    }

    //handle dirs
    if(dir != NULL && !master_done){
        //if reg file is found, add it to queue
        dir_rec(dir, Q, delay);
    }

    enqueue(Q, "DONE");

    //master_done = 1;

    //wait for threads
    for(int i=0; i<dim; i++){
        if((e = pthread_join(thread_pool[i], NULL))){
            fprintf(stderr, "[MASTER] pthread_join %d - %d\n", i, e);
            exit(-1);
        }
    }

if(!master_done){
master_done = 1;
    if((e = pthread_kill(sighandler_thread, SIGCONT))){
        fprintf(stderr, "[MASTER] pthread_kill - %d\n", e);
        exit(-1);
    }
}

    if((e = pthread_join(sighandler_thread, NULL))){
        fprintf(stderr, "[MASTER] pthread_join - %d\n", e);
        exit(-1);
    }
    
    //write in worker_file
    FILE *fd;
    fd = fopen(worker_file, "w");
    if(fd == NULL){
        perror("MASTER fopen - ");
    }
    else{
        fprintf(fd, "number of threads at exit: %d\n", dim);
        fclose(fd);
    }
    
    del(Q);

    free(Q);
    free(thread_pool);

}