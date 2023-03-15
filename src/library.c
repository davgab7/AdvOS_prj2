#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>

#include "shared_mem.h"

#define MAX_FILENAME_LEN 256

int pid;
Request request;
Response response;
struct timeval start, end, res_time;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        exit(1);
    }
    pid = getpid();

    int queue_id = get_sh_queue(SHM_NAME, 1);
    if (queue_id == -1) {
        printf("Error occured\n"); return -1;
    }

    // for (int j=0; j <=50; j++) {
    // char tmp[20];
    // sprintf(tmp, "msg %d", j);
    // strcpy(request.filename, tmp);

    // Open the file for reading
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Error opening file.\n");
        exit(1);
    }

    // Determine the size of the file
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("Size of provided file: %ld\n", size);

    char *buffer = (char *)malloc(2*size);
    if (buffer == NULL) {
        printf("Error allocating memory.\n");
        exit(1);
    }
    // Read the file contents into memory
    fread(buffer, 1, size, file);

    //add req to the queue
    request.mtype = 1;
    request.filesize = size;
    request.unique_id = pid;
    if (msgsnd(queue_id, &request, sizeof(Request), 0) == -1) {
        printf("Error: Failed to send message. queue_id=%d, errno=%d\n", queue_id, errno);
    }

    int unique_id = get_sh_queue(SHM_NAME, pid);
    if (unique_id == -1) {printf("Error occured\n"); return -1;}

    printf("waiting on server!\n");
    // Wait for a request to arrive on the message queue
    if (msgrcv(unique_id, &response, sizeof(Response), 1, 0) == -1) { // 0 for blocking, IPC_NOWAIT for non-blocking
    // No request was found on the message queue; sleep for a bit
         printf("Error:");
    }

    printf("Got permission to send in all units!\n");
    printf("mem_size : %d\n", response.mem_size);

    //Start timer
    gettimeofday(&start,NULL);

    if (response.ready == 0) {printf("Error: server not ready"); exit(-1);}

    char *block = attach_mem_block(SHM_NAME, SHM_SIZE, 1);
    if (block == NULL) {
        printf("Error occured\n"); return -1;
    }
    if (size < response.mem_size) {
        //Now put file in the shared mem
        printf("Attempting to write to shared_mem\n");
        strncpy(block, buffer, SHM_SIZE);
    } else {
        //TODO need chuncking
        //size_t size_of_chunk;
        // for (int i=0; i<size; i+=size_of_chunk) {
        //     //send(id, buffer+i, flags);
        // }
    }

    request.mtype = 1;
    request.filesize = size;
    request.file_uploaded = 1;
    if (msgsnd(unique_id, &request, sizeof(Request), 0) == -1) {
        printf("Error: Failed to send message. queue_id=%d, errno=%d\n", queue_id, errno);
    }

    printf("waiting on server!\n");
    // Wait for a request to compress
    if (msgrcv(unique_id, &response, sizeof(Response), 1, 0) == -1) { // 0 for blocking, IPC_NOWAIT for non-blocking
    // No request was found on the message queue; sleep for a bit
         printf("Error:");
    }

    printf("Got permission to download!\n");

    if (response.ready == 0) {printf("Error: server not ready"); exit(-1);}

    //printf("Local: %s", buffer);
    strncpy(buffer, block, 2*size);

    if (msgsnd(unique_id, &request, sizeof(Request), 0) == -1) {
        printf("Error: Failed to send message. queue_id=%d, errno=%d\n", queue_id, errno);
    }

    //printf("This is what I got\n %s\n", buffer);

    //Stop timer
    gettimeofday(&end,NULL);
    timersub(&end, &start, &res_time);
    printf("=> CST: %lu microseconds\n", (res_time.tv_sec*1000000L+res_time.tv_usec));

    // Open a new file for writing
    char newfile_name[100];
    sprintf(newfile_name, "%s_compressed", argv[1]);
    FILE *newfile = fopen(newfile_name, "w");
    if (newfile == NULL) {printf("Error opening new file.\n");}

    // Write the contents of the buffer to the new file
    fwrite(buffer, 1, 2*size, newfile);

    // Cleanup
    dettach_mem_block(block);
    free(buffer);
    fclose(file);
    fclose(newfile);

    return 0;
}

