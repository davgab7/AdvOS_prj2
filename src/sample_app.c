#include "library.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct timeval start, end, res_time;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <filename> <SYNC/ASYNC>\n", argv[0]);
        exit(1);
    }

    //Start timer
    gettimeofday(&start,NULL);
    int pid = getpid();

    if (strcmp(argv[2], "SYNC") == 0) {sync_call(argv[1], pid);}
    else if (strcmp(argv[2], "ASYNC") == 0) {async_call(argv[1], pid);}
    else {
        printf("Usage: %s <filename> <SYNC/ASYNC>\n", argv[0]);
        exit(1);
    }

    //Stop timer
    gettimeofday(&end,NULL);
    timersub(&end, &start, &res_time);
    printf("\n\nDONE\n===> CST: %lu microseconds\n\n", (res_time.tv_sec*1000000L+res_time.tv_usec));

}

