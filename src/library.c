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
#include <semaphore.h>
#include <pthread.h>

#include "shared_mem.h"

int num_chunks, pid_async;
Request request;
Response response;
size_t filesize, mem_size, compressed_size;
char *filename_async;

static void *parallel_function() {
    int queue_id = get_sh_queue(SHM_NAME, 1);
    if (queue_id == -1) {
        printf("Error occured\n"); exit(1);
    }

    // open semaphores
    sem_t* writer_sem = attach_semaphore(WRITER_SEM_NAME);
    sem_t* reader_sem = attach_semaphore(READER_SEM_NAME);

    // Open the file for reading
    FILE *file = fopen(filename_async, "r");
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
    request.unique_id = pid_async;
    if (msgsnd(queue_id, &request, sizeof(Request), 0) == -1) {
        printf("Error: Failed to send message. queue_id=%d, errno=%d\n", queue_id, errno);
    }

    int unique_id = get_sh_queue(SHM_NAME, pid_async);
    if (unique_id == -1) {printf("Error occured\n"); exit(1);}

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



    if (response.ready == 0) {printf("Error: server not ready"); exit(-1);}

    char *block = attach_mem_block(SHM_NAME, mem_size, 1);
    if (block == NULL) {
        printf("Error occured\n"); exit(1);
    }

    //Send file to server in chunks
    for (int i = 0; i < num_chunks; i++) {
        //printf("Attempting to write chunk_%d to shared_mem\n", i);
        sem_wait(reader_sem);
        strncpy(block, buffer + (mem_size*i), mem_size);
        sem_post(writer_sem);
    } 

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
    strncpy(buffer, block, compressed_size);

    // 5 ===============================================
    if (msgsnd(unique_id, &request, sizeof(Request), 0) == -1) {
        printf("Error: Failed to send message. queue_id=%d, errno=%d\n", queue_id, errno);
    }

    //printf("This is what I got\n %s\n", buffer);

    // Open a new file for writing
    char newfile_name[100];
    sprintf(newfile_name, "%s_compressed", filename_async);
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

    pthread_exit(NULL);
}

//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================
//===========================================================================================================================


void sync_call(char *filename, int pid) {
    printf("Starting SYNC CALL for %s\n", filename);

    int queue_id = get_sh_queue(SHM_NAME, 1);
    if (queue_id == -1) {
        printf("Error occured\n"); exit(1);
    }

    // open semaphores
    sem_t* writer_sem = attach_semaphore(WRITER_SEM_NAME);
    sem_t* reader_sem = attach_semaphore(READER_SEM_NAME);

    // Open the file for reading
    FILE *file = fopen(filename, "r");
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
    if (unique_id == -1) {printf("Error occured\n"); exit(1);}

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



    if (response.ready == 0) {printf("Error: server not ready"); exit(-1);}

    char *block = attach_mem_block(SHM_NAME, mem_size, 1);
    if (block == NULL) {
        printf("Error occured\n"); exit(1);
    }

    //Send file to server in chunks
    for (int i = 0; i < num_chunks; i++) {
        //printf("Attempting to write chunk_%d to shared_mem\n", i);
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
    strncpy(buffer, block, compressed_size);

    // 5 ===============================================
    if (msgsnd(unique_id, &request, sizeof(Request), 0) == -1) {
        printf("Error: Failed to send message. queue_id=%d, errno=%d\n", queue_id, errno);
    }

    //printf("This is what I got\n %s\n", buffer);

    // Open a new file for writing
    char newfile_name[100];
    sprintf(newfile_name, "%s_compressed", filename);
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
}

void async_call(char *filename, int pid) {
    printf("Starting ASYNC CALL\n");

    filename_async = filename;
    pid_async = pid;

    pthread_t thread_id;
    
    // Create the new thread and pass it a pointer to the 'value' variable
    if (pthread_create(&thread_id, NULL, parallel_function, NULL) != 0) {
        printf("Failed to create thread.\n");
        exit(1);
    }

    for (int i=0; i < 50; i++) {
        printf("I am not blocked! But request is being processed! we will join the thread in ... %d/50\n\n", i);
    }
    
    // Wait for the new thread to finish executing
    if (pthread_join(thread_id, NULL) != 0) {
        printf("Failed to join thread.\n");
        exit(1);
    }

    printf("compressed file was recieved and saved\n");
    printf("compressed_size-> %ld\n", compressed_size);
}
