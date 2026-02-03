#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *a){fprintf(stderr,"Usage: %s <pid>\n",a); exit(1);}
static int isnum(const char*s){for(;*s;s++) if(!isdigit(*s)) return 0; return 1;}

static void open_fail(const char *path) {
    if (errno == ENOENT) fprintf(stderr, "Error: PID not found (%s)\n", path);
    else if (errno == EACCES) fprintf(stderr, "Error: Permission denied (%s)\n", path);
    else{ perror(path);}
    exit(1);
}

//cmdline
static void print_cmdline(const char *cmdp){
    FILE *f = fopen(cmdp, "rb");
    if (!f) open_fail(cmdp);

    char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);

    if (n == 0){
        //processes with empty cmdline
        printf("Cmd: \n");
        return;
    }

    //replace '\0' with spaces
    for(size_t i = 0; i < n; i++){
        if (buf[i] == '\0') buf[i] = ' ';
    }

    buf[n] = '\0';

    while (n > 0 && buf[n-1] == ' '){
        buf[n-1] = '\0';
        n--;
    }

    printf("Cmd: %s\n", buf);
}

//VmRSS
static long read_vmrss_kb(const char *statusp){
    FILE *f = fopen(statusp, "r");
    if (!f) open_fail(statusp);

    char line[512];
    long vmrss = -1;

    while(fgets(line, sizeof(line), f)){
        if (strncmp(line, "VmRSS:", 6) == 0){
            if (sscanf(line + 6, "%ld", &vmrss) == 1){
                break;
            }
        }
    }

    fclose(f);
    return vmrss;
}

//state and PPID
static void read_state_ppid(const char *statp, char *state_out, long *ppid_out){
    FILE *f = fopen(statp, "r");
    if (!f) open_fail(statp);

    char line[4096];
    if (!fgets(line, sizeof(line), f)){
        fclose(f);
        fprintf(stderr, "Error: could not read %s\n", statp);
        exit(1);
    }
    fclose(f);


    //finding the end of the comm
    char *rparen = strrchr(line, ')');
    if(!rparen){
        fprintf(stderr, "Error: malformed stat file (missing ')')\n");
        exit(1);
    }

    char state = '\0';
    long ppid = -1;

    if(sscanf(rparen + 1, " %c %ld", &state, &ppid) != 2){
        fprintf(stderr, "Error: malformed stat file (could not parse state/ppid)\n");
        exit(1);
    }

    *state_out = state;
    *ppid_out = ppid;
}

// CPU time
static double read_cpu_seconds(const char *statp){
    FILE *f = fopen(statp, "r");
    if (!f) open_fail(statp);

    char line[4096];
    if (!fgets(line, sizeof(line), f)){
        fclose(f);
        fprintf(stderr, "Error: could not read %s\n", statp);
        exit(1);
    }
    fclose(f);

    char *rparen = strrchr(line, ')');
    if(!rparen){
        fprintf(stderr, "Error: malformed stat file (missing ')')\n");
        exit(1);
    }

    unsigned long long utime = 0, stime = 0;

    int n = sscanf(rparen + 1,
                    " %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %llu %llu",
                    &utime, &stime);
    
    if (n != 2){
        fprintf(stderr, "Error: malformed stat file (could not parse utime/stime)\n");
        exit(1);
    }

    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    if(ticks_per_sec <= 0) ticks_per_sec = 100;

    return(utime + stime) / (double)ticks_per_sec;
}


// MAIN
int main(int c,char**v){
    if(c!=2||!isnum(v[1])) usage(v[0]);
    const char *pid = v[1];

    char statp[256], statusp[256], cmdp[256];
    snprintf(statp, sizeof(statp), "/proc/%s/stat", pid);
    snprintf(statusp, sizeof(statusp), "/proc/%s/status",pid);
    snprintf(cmdp, sizeof(cmdp), "/proc/%s/cmdline", pid);

    printf("PID: %s\n", pid);
    print_cmdline(cmdp);
///////////
    long rss = read_vmrss_kb(statusp);
    if (rss>0) printf("VmRSS: %ld kB\n", rss);
    else printf("VmRSS: (not found)\n");
//////////
    char state;
    long ppid;
    read_state_ppid(statp, &state, &ppid);

    printf("State: %c\n", state);
    printf("PPID: %ld\n", ppid);
//////////
    double cpu = read_cpu_seconds(statp);
    printf("CPU: %.3f\n", cpu);

    return 0;
}