#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "../snappy-c/snappy.h"
#include "shared_mem.h"

#define COMPRESS_MAX_SIZE 2621444

Request request;
Response response;
size_t filesize, chunk_size;
int total_mem_size, num_chunks;

// pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t queue_cv = PTHREAD_COND_INITIALIZER;

// function prototypes
char* handle_request(char *file, size_t file_size);

void sigint_handler(int sig) {
    printf("Received SIGINT signal (%d).\n", sig);
    // Insert your code to execute here
    sem_unlink(WRITER_SEM_NAME);
    sem_unlink(READER_SEM_NAME);
    destroy_sh_queue(SHM_NAME, 1);
    destroy_mem_block(SHM_NAME, 1);
    exit(1);
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <n_sms>, <sms_size>\n", argv[0]);
        exit(1);
    }
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    if (a <= 0 || b <= 0) {
        printf("Both arguments must be positive integers.\n");
        exit(1);
    }

    total_mem_size = a*b;

    //Clean up 
    destroy_mem_block(SHM_NAME, 1);
    destroy_sh_queue(SHM_NAME, 1);
    sem_unlink(WRITER_SEM_NAME);
    sem_unlink(READER_SEM_NAME);

    int queue_id = get_sh_queue(SHM_NAME, 1);
    if (queue_id == -1) {printf("Error occured when creating queue\n"); return -1;}

    char *block = attach_mem_block(SHM_NAME, total_mem_size, 1);
    if (block == NULL) {
        printf("Error occured\n"); return -1;
    }

    sem_t* writer_sem = create_semaphore(WRITER_SEM_NAME, 0);
    sem_t* reader_sem = create_semaphore(READER_SEM_NAME, 1);
    
    char *buffer = (char *)malloc(total_mem_size);
    if (buffer == NULL) {
        printf("Error allocating memory.\n");
        exit(1);
    }
    // Set up the signal handler for SIGINT
    signal(SIGINT, sigint_handler);


    //TIME
    //TO
    //RUN
    while (1) {
        printf("Server running!\n");
        // 1 ===============================================
        // Wait for a request to arrive on the message queue
        if (msgrcv(queue_id, &request, sizeof(Request), 1, 0) == -1) { // 0 for blocking, IPC_NOWAIT for non-blocking
        // No request was found on the message queue; sleep for a bit
            sleep(1);
            continue;
        }
        
        int unique_id_num = request.unique_id;
        int unique_id = get_sh_queue(SHM_NAME, unique_id_num);
        if (unique_id == -1) {printf("Error occured while joining unique queue\n"); return -1;}

        // 2 ===============================================
        response.mtype = 1;
        response.mem_size = total_mem_size;
        response.ready = 1;
        if (msgsnd(unique_id, &response, sizeof(Response), 0) == -1) {
            printf("Error: Failed to send message. queue_id=%d, errno=%d\n", unique_id, errno);
        }

        // // 3 ===============================================
        // if (msgrcv(unique_id, &request, sizeof(Request), 1, 0) == -1) {printf("Error:");}

        filesize = request.filesize;
        num_chunks = (int)(filesize / total_mem_size)+1;
        char* uncompressed_file = (char *)malloc(filesize+1000);
        printf("\nchunk # : %d\n", num_chunks);
        printf("filesize # : %ld\n", filesize);

        //Recieve file from client
        for (int i = 0; i < num_chunks; i++) {
            //printf("Attempting to read chunk_%d from shared_mem\n", i);
            sem_wait(writer_sem);
            int x = filesize - (total_mem_size*(i));
            if (i == num_chunks - 1) strncpy(uncompressed_file + (total_mem_size*i), block, x);
            else strncpy(uncompressed_file + (total_mem_size*i), block, total_mem_size);
            sem_post(reader_sem);
        } 

        //printf("This is what I got\n %s\n", uncompressed_file);

        char* compressed_file = handle_request(uncompressed_file, filesize);
        if (compressed_file == NULL) exit(1); 
        //printf("This is what I made\n %s\n", compressed_file);

        //Copy compressed into buffer
        strncpy(block, compressed_file, total_mem_size);

        // 4 ===============================================
        response.mtype = 1;
        response.mem_size = total_mem_size;
        response.ready = 1;
        if (msgsnd(unique_id, &response, sizeof(Response), 0) == -1) {
            printf("Error: Failed to send message. queue_id=%d, errno=%d\n", unique_id, errno);
        }

        //Wait make sure client is done and then clean shared mem
        // 5 ===============================================
        if (msgrcv(unique_id, &request, sizeof(Request), 1, 0) == -1) {printf("Error:");}
        destroy_sh_queue(SHM_NAME, unique_id_num); //Destory unique queue
        memset(block, 0, total_mem_size); //Clear shared mem
    }

    sem_close(writer_sem);
    sem_close(reader_sem);
    sem_unlink(WRITER_SEM_NAME);
    sem_unlink(READER_SEM_NAME);
    destroy_sh_queue(SHM_NAME, 1);
    dettach_mem_block(block);
    destroy_mem_block(SHM_NAME, 1);
    free(buffer);

}

char* handle_request(char *file, size_t file_size) {
    // compress the file content
    char *compressed  = (char *)malloc(2*file_size);
    response.compressed_size = snappy_max_compressed_length(file_size);
    printf("max_comp_size : %ld\n", response.compressed_size);
    struct snappy_env env;
    snappy_init_env(&env);
    if (snappy_compress(&env, file, file_size, compressed, &response.compressed_size) != 0) {
        printf("strncppy failed"); return NULL;
    }
    printf("compressed_size-> %ld\n\n", response.compressed_size);
    //printf("This is what I made\n %.*s\n", (int)compressed_size, compressed);
    return compressed;
}


















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
