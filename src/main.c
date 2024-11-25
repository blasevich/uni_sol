#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/wait.h>

#define MAX_LEN_PATH 256

#define UNIX_PATH_MAX 108
#define SOCKNAME "./farm.sck"

void master();
void collector();

//////////////////////////////////////////////

int _collector_done(){
    //sends DONE to collector
    char done[] = "DONE";

    int fd_skt;
    struct sockaddr_un sa;
    int left = strlen(done), r;
    char* tobesent;
    long dummy_res = 0;
    
    strncpy(sa.sun_path, SOCKNAME, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;

    fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);
    if(fd_skt == -1){
        perror("main socket - ");
        return -1;
    }

    while(connect(fd_skt, (struct sockaddr*)&sa, sizeof(sa)) == -1){
        if(errno == ENOENT)
            sleep(1);
        else{
            perror("main connect - ");
            return -1;
        }
    }

    //send result
    left = sizeof(long);
    tobesent = (char*)&dummy_res;
    while(left > 0){
        if((r = write(fd_skt, tobesent, left)) == -1){
            perror("main write 1 - ");
            return -1;
        }

        left = left - r;
        tobesent = tobesent + r;
    }

    //send str length
    left = sizeof(int);
    int len = strlen(done);
    tobesent = (char*)&len;
    while(left > 0){
        if((r = write(fd_skt, tobesent, left)) == -1){
            perror("main write 2 - ");
            return -1;
        }

        left = left - r;
        tobesent = tobesent + r;
    }

    //send 'filename'
    left = strlen(done);
    tobesent = done;
    while(left > 0){
        if((r = write(fd_skt, tobesent, left)) == -1){
            perror("main write 3 - ");
            return -1;
        }

        left = left - r;
        tobesent = tobesent + r;
    }

    if(close(fd_skt) == -1){
        perror("main close - ");
        return -1;
    }

    return 0;

}

//////////////////////////////////////////////

int main(int argc, char* argv[]){
    int opt, i;

    int N = 4;
    int Q = 8;
    int T = 0;
    char* dir_name = NULL;
    char **files = NULL;

    while((opt = getopt(argc, argv, "n:q:t:d:h")) != -1){
        switch(opt){
            case 'n':
                N = atoi(optarg);
                break;
            case 'q':
                Q = atoi(optarg);
                if(Q==0){
                    printf("Q must be at least 1\n");
                    exit(-1);
                }
                break;
            case 't':
                T = atoi(optarg)*1000;
                break;
            case 'd':
                dir_name = optarg;
                break;                
            case 'h':
                printf("[MAIN] %s: -n <worker threads number> -q <queue length> -t <delay> -d <directory>\n", argv[0]);
                exit(-1);
                break;                
            case ':':
                //printf("-%c: missing argument\n", optopt);
                exit(-1);
                break;                
            //case '?':
            //    printf("-%c: not a valid option\n", optopt);
            //    break;                
            default:
                printf("[MAIN] %s: -n <worker threads number> -q <queue length> -t <delay> -d <directory>\n", argv[0]);
                exit(-1);
                break;
        }
    }

    int num_files = argc - optind; //optind index of next element to be processed in argv
    int j=0;
    if(num_files > 0){
        files = malloc(num_files*sizeof(char*));
        if(files == NULL){
            perror("MAIN malloc 1 - ");
            exit(-1);
        }
        
        //int j=0;
        for(i=optind; i<argc; i++){
            files[j] = malloc(MAX_LEN_PATH*sizeof(char));
            if(files[j] == NULL){
                perror("MAIN malloc 2 - ");
                exit(-1);
            }

            strcpy(files[j], argv[i]);

            j++;
        }
    }

    pid_t pid = fork();
    if(pid<0){
        perror("main fork - ");
        return -1;
    }
    else if(pid == 0){
        //child process
        collector();

        if(num_files > 0){
            for(i=0; i<j; i++){
                free(files[i]);
            }
            free(files);
        }
    }
    else{
        //master
        master(N, Q, T, dir_name, files, num_files);

        _collector_done();

        //wait pid
        if(waitpid(pid, 0, 0) == -1){
            perror("main waitpid - ");
        }

        if(num_files > 0){
            for(i=0; i<j; i++){
                free(files[i]);
            }
            free(files);
        }

        //remove farm.sck
        if(remove(SOCKNAME) == -1){
            perror("main remove - ");
        }
    }

    return 0;
}