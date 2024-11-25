#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "../include/aux.h"
#include "../include/queue.h"

#define MAX_LEN_PATH 256

int is_reg(char* name){
   
    struct stat s;
    if(stat(name, &s) == -1){
        perror("aux stat - ");
        return -1;
    }
    
    if(S_ISREG(s.st_mode))
        return 1;
    else
        return 0;

}

void dir_rec(char *name, queue * Q, int delay){
    DIR* dir;
    
    if((dir = opendir(name)) == NULL){
        perror("AUX opendir - ");
        return;
    }
    
    struct dirent* elem;
    char path[MAX_LEN_PATH];
    char* new_file;
    
     while((errno = 0, elem = readdir(dir)) != NULL && !master_done){
        if(errno != 0){
            perror("aux readdir - ");
            return;
        }
        if(elem->d_type == DT_REG){
            char f[MAX_LEN_PATH];

             strcpy(f, name);
             strcat(f, "/");
             strcat(f, elem->d_name);

            new_file = malloc(strlen(f) + 1);
            strcpy(new_file, f);

            enqueue(Q, new_file);
            free(new_file);
            //wait
            if(usleep(delay) == -1){
                perror("aux usleep - ");
            }
        }
        else if(elem->d_type == DT_DIR && strcmp(elem->d_name, ".") != 0 && strcmp(elem->d_name, "..") != 0){             
            snprintf(path, sizeof(path)+1, "%s/%s", name, elem->d_name);
            dir_rec(path, Q, delay);
        }
    }
    
    if(closedir(dir) == -1){
        perror("aux closedir - ");
    }
}

int minus_one(int res){
    if(res == -1){
        perror("");
        return 1;
    }

    return 0;
}