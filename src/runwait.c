#include "common.h"
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

static void usage(const char *a){fprintf(stderr,"Usage: %s <cmd> [args]\n",a); exit(1);}
static double d(struct timespec a, struct timespec b){
 return (b.tv_sec-a.tv_sec)+(b.tv_nsec-a.tv_nsec)/1e9;}
int main(int c,char**v){
    if (c < 2) usage(v[0]);

    struct timespec t0, t1;
    if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0) {
        perror("clock_gettime");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0){
        perror("fork");
        exit(1);
    }

    if (pid == 0){
        //child
        execvp(v[1], &v[1]);
        //runs only if execvp fails
        perror("execvp");
        _exit(127);
    }

    //parent
    int status = 0;
    pid_t w = waitpid(pid, &status, 0);
    if (w<0){
        perror("waitpid");
        exit(1);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &t1) !=0){
        perror("clock_gettime");
        exit(1);
    }
    double elapsed = d(t0, t1);

    if(WIFEXITED(status)){
        printf("pid=%d exit=%d time=%.6f\n", (int)pid, WEXITSTATUS(status), elapsed);
    } else if (WIFSIGNALED(status)){
        printf("pid=%d signal=%d time=%.6f\n", (int)pid, WTERMSIG(status), elapsed);
    } else {
        printf("pid=%d exit=? time=%.6f\n", (int)pid, elapsed);
    }

    return 0;
}
