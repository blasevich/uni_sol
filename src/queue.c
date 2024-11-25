#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/queue.h"
#include "../include/aux.h"

#define MAX_LEN_PATH 256

void check_error(int e){
    if(e){
        fprintf(stderr, "queue pthread - %d\n", e);
    }
}

int init(queue *Q, int max_len){
    Q->max_len = max_len+1;
    Q->dim = 0;

    elem* tmp = malloc(sizeof(elem));
    if(tmp == NULL){
        perror("enqueue malloc 1 - ");
        return -1;
    }

    tmp->next = NULL;
    Q->first = tmp;
    Q->last = tmp;

    check_error(pthread_mutex_init(&Q->mutex, NULL));

    check_error(pthread_cond_init(&Q->full, NULL));
    check_error(pthread_cond_init(&Q->empty, NULL));

    return 0;

}

int isEmpty(queue *Q){
    return Q->dim == 0;
}

int isFull(queue *Q){
    return Q->dim == Q->max_len;
}

int queue_size(queue *Q){   
    return Q->dim;
}

int enqueue(queue *Q, char fname[]){
    if(fname != NULL){
        check_error(pthread_mutex_lock(&Q->mutex));

        while(isFull(Q)){
            check_error(pthread_cond_wait(&Q->full, &Q->mutex));
        }

        elem *new = malloc(sizeof(elem));
        if(new == NULL){
            perror("enqueue malloc 2 - ");
            return -1;
        }

        strcpy(new->fname, fname);
        new->next = NULL;

        Q->last->next = new;
        Q->last = new;

        if(strcmp(new->fname, "DELETE") != 0)
            Q->dim++;

        check_error(pthread_cond_broadcast(&Q->empty));

        check_error(pthread_mutex_unlock(&Q->mutex));

        return 0;
    }
    else{
        return -1;
    }
}

int dequeue(queue *Q, char fname[]){
    check_error(pthread_mutex_lock(&Q->mutex));

    while(isEmpty(Q)){
        check_error(pthread_cond_wait(&Q->empty, &Q->mutex));
    }

    elem* old = Q->first;
    elem* new_first = old->next;

    if(new_first == NULL){
        check_error(pthread_mutex_unlock(&Q->mutex));
        return -1;
    }

    if(strcmp(new_first->fname, "DONE") == 0){
        strcpy(fname, new_first->fname);
    }
    else{
        strcpy(fname, new_first->fname);
        Q->first = new_first;

        if(strcmp(new_first->fname, "DELETE") != 0)
            Q->dim--;

        check_error(pthread_cond_broadcast(&Q->full));

        free(old);
    }

    check_error(pthread_mutex_unlock(&Q->mutex));

    return 0;
}

void printQueue(queue *Q){
    if(isEmpty(Q))
        printf("empty\n");
    else{
        elem *tmp = Q->first->next;

        printf("----------------------\n");

        while(tmp != NULL){
            printf("%s\n", tmp->fname);
            tmp = tmp->next;
        }

        printf("----------end----------\n");
    }
}

void del(queue *Q){

    if(Q->first != NULL)
        free(Q->first);
    
    if(Q->last != NULL)
        free(Q->last);

    check_error(pthread_cond_destroy(&Q->full));
    check_error(pthread_cond_destroy(&Q->empty));

    check_error(pthread_mutex_destroy(&Q->mutex));
}

