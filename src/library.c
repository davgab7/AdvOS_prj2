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
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h> 

#include "shared_mem.h"

#define MAX_FILENAME_LEN 256

int pid, num_chunks;
Request request;
Response response;
struct timeval start, end, res_time;
size_t filesize, mem_size, compressed_size;

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

    sem_t* writer_sem = attach_semaphore(WRITER_SEM_NAME);
    sem_t* reader_sem = attach_semaphore(READER_SEM_NAME);

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
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    printf("Size of provided file: %ld\n", filesize);

    char *buffer = (char *)malloc(2*filesize);
    if (buffer == NULL) {
        printf("Error allocating memory.\n");
        exit(1);
    }
    // Read the file contents into memory
    fread(buffer, 1, filesize, file);

    //add req to the queue
    // 1 ===============================================
    request.mtype = 1;
    request.filesize = (size_t) filesize;
    request.unique_id = pid;
    if (msgsnd(queue_id, &request, sizeof(Request), 0) == -1) {
        printf("Error: Failed to send message. queue_id=%d, errno=%d\n", queue_id, errno);
    }

    int unique_id = get_sh_queue(SHM_NAME, pid);
    if (unique_id == -1) {printf("Error occured\n"); return -1;}

    printf("waiting on server!\n");
    // Wait for a request to arrive on the message queue
    // 2 ===============================================
    if (msgrcv(unique_id, &response, sizeof(Response), 1, 0) == -1) { // 0 for blocking, IPC_NOWAIT for non-blocking
    // No request was found on the message queue; sleep for a bit
         printf("Error:");
    }

    printf("Got permission to send in all units!\n");
    //printf("mem_size : %d\n", response.mem_size);
    mem_size = response.mem_size;
    num_chunks = (int)(filesize / mem_size)+1;
    printf("chunk # : %d\n", num_chunks);

    //Start timer
    gettimeofday(&start,NULL);

    if (response.ready == 0) {printf("Error: server not ready"); exit(-1);}

    char *block = attach_mem_block(SHM_NAME, SHM_SIZE, 1);
    if (block == NULL) {
        printf("Error occured\n"); return -1;
    }

    //Send file to server in chunks
    for (int i = 0; i < num_chunks; i++) {
        printf("Attempting to write chunk_%d to shared_mem\n", i);
        sem_wait(reader_sem);
        strncpy(block, buffer + (mem_size*i), mem_size);
        sem_post(writer_sem);
    } 

    // // 3 ===============================================
    // request.mtype = 1;
    // request.filesize = filesize;
    // request.file_uploaded = 1;
    // if (msgsnd(unique_id, &request, sizeof(Request), 0) == -1) {
    //     printf("Error: Failed to send message. queue_id=%d, errno=%d\n", queue_id, errno);
    // }

    printf("waiting on server!\n");
    // Wait for a request to compress
    // 4 ===============================================
    if (msgrcv(unique_id, &response, sizeof(Response), 1, 0) == -1) { // 0 for blocking, IPC_NOWAIT for non-blocking
    // No request was found on the message queue; sleep for a bit
         printf("Error:");
    }

    printf("Got permission to download!\n");

    if (response.ready == 0) {printf("Error: server not ready"); exit(-1);}

    //printf("Local: %s", buffer);
    compressed_size = response.compressed_size;
    printf("compressed_size-> %ld\n", compressed_size);
    strncpy(buffer, block, 2*filesize);

    // 5 ===============================================
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
    fwrite(buffer, 1, compressed_size, newfile);

    // Cleanup
    sem_close(writer_sem);
    sem_close(reader_sem);
    dettach_mem_block(block);
    free(buffer);
    fclose(file);
    fclose(newfile);

    return 0;
}

