#ifndef QUEUE_H_
#define QUEUE_H_

#include <pthread.h>

typedef struct elem{
    struct elem* next;
    char fname[256];
}elem;

typedef struct queue{
    int max_len;
    int dim;
    elem *first;
    elem *last;
    pthread_mutex_t mutex;
    pthread_cond_t empty;
    pthread_cond_t full;
}queue;

void check_error(int e);

int init(queue *Q, int max_len);

int isEmpty(queue *Q);

int isFull(queue *Q);

int queue_size(queue *Q);

int enqueue(queue *Q, char fname[]);

int dequeue(queue *Q, char fname[]);

void printQueue(queue *Q);

void del(queue *Q);

#endif