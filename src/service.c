#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
//#include "../snappy-c/snappy.h"
#include "shared_mem.h"

#define QUEUE_MAX_SIZE 20


// global variables for shared memory
//int total_mem_size;

Request request;
Response response;

// global variables for queue
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cv = PTHREAD_COND_INITIALIZER;

// function prototypes
//void handle_request(Request request);

int main(int argc, char *argv[]) {
    // if (argc != 3) {
    //     printf("Usage: %s <n_sms>, <sms_size>\n", argv[0]);
    //     exit(1);
    // }
    int total_mem_size = SHM_SIZE;

    destroy_mem_block(SHM_NAME, 1);

    int queue_id = get_sh_queue(SHM_NAME, 1);
    if (queue_id == -1) {printf("Error occured when creating queue\n"); return -1;}

    char *block = attach_mem_block(SHM_NAME, SHM_SIZE, 1);
    if (block == NULL) {
        printf("Error occured\n"); return -1;
    }

    char *buffer = (char *)malloc(total_mem_size);
    if (buffer == NULL) {
        printf("Error allocating memory.\n");
        exit(1);
    }

    while (1) {
        printf("Server running!\n");
        // Wait for a request to arrive on the message queue
        if (msgrcv(queue_id, &request, sizeof(Request), 1, 0) == -1) { // 0 for blocking, IPC_NOWAIT for non-blocking
        // No request was found on the message queue; sleep for a bit
            sleep(1);
            continue;
        }
        
        int unique_id = get_sh_queue(SHM_NAME, request.unique_id);
        if (unique_id == -1) {printf("Error occured while joining unique queue\n"); return -1;}

        response.mtype = 1;
        response.mem_size = total_mem_size;
        response.ready = 1;
        if (msgsnd(unique_id, &response, sizeof(Response), 0) == -1) {
            printf("Error: Failed to send message. queue_id=%d, errno=%d\n", unique_id, errno);
        }

        if (msgrcv(unique_id, &request, sizeof(Request), 1, 0) == -1) {printf("Error:");}

        printf("This is what I got\n %s\n", block);

        strncpy(block+5, "XXXX", 4);
        printf("This is what I made\n %s\n", block);

        response.mtype = 1;
        response.mem_size = total_mem_size;
        response.ready = 1;
        if (msgsnd(unique_id, &response, sizeof(Response), 0) == -1) {
            printf("Error: Failed to send message. queue_id=%d, errno=%d\n", unique_id, errno);
        }

        //Wait make usre clinet is done and then clean shared mem
        if (msgrcv(unique_id, &request, sizeof(Request), 1, 0) == -1) {printf("Error:");}
        memset(block, 0, total_mem_size);
    }

    
    dettach_mem_block(block);
    destroy_mem_block(SHM_NAME, 1);


    // // Create the semaphore
    // sem_id = semget(key, 1, IPC_CREAT | 0666);
    // if (sem_id == -1) {
    //     perror("semget");
    //     exit(1);
    // }

    // // Initialize the semaphore to 1 (i.e., unlocked)
    // semctl(sem_id, 0, SETVAL, 1);



    // // Loop indefinitely, servicing client requests
    // while (1) {
    //     // Wait for a request to arrive on the message queue
    //     if (msgrcv(msgq_id, &request, sizeof(Request), 1, IPC_NOWAIT) == -1) {
    //     // No request was found on the message queue; sleep for a bit
    //     usleep(10000);
    //     continue;
    //     }

    //     // A request was found on the message queue; try to acquire the semaphore
    //     if (semctl(sem_id, 0, GETVAL, 0) == 0) {
    //         // The semaphore is locked; sleep for a bit and try again later
    //         usleep(10000);
    //         continue;
    //     }

    //     // Lock the semaphore
    //     struct sembuf sembuf;
    //     sembuf.sem_num = 0;
    //     sembuf.sem_op = -1;
    //     sembuf.sem_flg = SEM_UNDO;
    //     semop(sem_id, &sembuf, 1);

        

    //     handle_request();
    //     // Send the response back to the client
    //     if (msgsnd(msgq_id, &response, sizeof(Response), 0) == -1) {
    //         perror("msgsnd");
    //     }

    //     // Unlock the semaphore
    //     sembuf.sem_op = 1;
    //     semop(sem_id, &sembuf, 1);
    // }

    

    // // Destroy the message queue, semaphore, and shared memory segment
    // if (semctl(sem_id, 0, IPC_RMID) == -1) {
    //     perror("semctl");
    //     exit(1);
    // }
    

    // return 0;
}


// void handle_request(Request request) {
//     // compress the file content
//     size_t compressed_size = snappy_max_compressed_length(strlen(request.filecontent));
//     if (snappy_compress(request.filecontent, strlen(request.filecontent), request.compressed_content, &compressed_size) != SNAPPY_OK) {
//         // if compression fails, send error response
//         Response response = {0};
//         response.success = 0;
//         strncpy(response.error_message, "Compression failed", sizeof(response.error_message));
//         strncpy(response.request.filename, request.filename, sizeof(response.request.filename));
//         strncpy(response.request.filecontent, request.filecontent, sizeof(response.request.filecontent));
//         response.request.compressed_size = compressed_size;
//         //strncpy(response.request.compressed_content, request.compressed_content, sizeof(response.request.compressed_content));
//     }
//     else {
//         Response response = {0};
//         response.success = 1;
//         strncpy(response.error_message, "Compression successful", sizeof(response.error_message));
//         strncpy(response.request.filename, request.filename, sizeof(response.request.filename));
//         strncpy(response.request.filecontent, request.filecontent, sizeof(response.request.filecontent));
//         response.request.compressed_size = compressed_size;
//         strncpy(response.request.compressed_content, request.compressed_content, sizeof(response.request.compressed_content));
//     }
// }

