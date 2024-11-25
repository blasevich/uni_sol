#ifndef AUXFUN_H_
#define AUXFUN_H_

#include "../include/queue.h"

extern int master_done;

int is_reg(char* name);

void dir_rec(char *name, queue * Q, int delay);

typedef struct res{ //result struct
    char file_name[256];
    long r;
}res;

#endif