#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include "snappy.h"

#define SHM_NAME "/tinyfile_shm"
#define SHM_SIZE 1024
#define QUEUE_MAX_SIZE 20

// define request structure
typedef struct {
    char filename[256];
    char filecontent[SHM_SIZE];
    int compressed_size;
    char compressed_content[SHM_SIZE];
} Request;

// define response structure
typedef struct {
    int success;
    char error_message[256];
    Request request;
} Response;

// global variables for shared memory
int shm_fd;
void* shm_ptr;

// global variables for queue
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cv = PTHREAD_COND_INITIALIZER;
int queue_size = 0;
Request queue[QUEUE_MAX_SIZE];

// function prototypes
void* worker_thread(void* arg);
void handle_request(Request request);
void handle_signal(int signal);

int main() {

    //parse args

    // create shared memory
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SHM_SIZE);
    shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // set up signal handling
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // start worker threads
    pthread_t threads[5];
    for (int i = 0; i < 5; i++) {
        if (pthread_create(&threads[i], NULL, worker_thread, NULL) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    // wait for worker threads to finish
    for (int i = 0; i < 5; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            exit(1);
        }
    }

    // destroy shared memory
    munmap(shm_ptr, SHM_SIZE);
    shm_unlink(SHM_NAME);

    return 0;
}

void* worker_thread(void* arg) {
    while (1) {
        // take a request off the queue
        pthread_mutex_lock(&queue_mutex);
        while (queue_size == 0) {
            pthread_cond_wait(&queue_cv, &queue_mutex);
        }
        Request request = queue[0];
        for (int i = 1; i < queue_size; i++) {
            queue[i - 1] = queue[i];
        }
        queue_size--;
        pthread_mutex_unlock(&queue_mutex);

        // handle the request
        handle_request(request);
    }
}

void handle_request(Request request) {
    // compress the file content
    size_t compressed_size = snappy_max_compressed_length(strlen(request.filecontent));
    if (snappy_compress(request.filecontent, strlen(request.filecontent), request.compressed_content, &compressed_size) != SNAPPY_OK) {
        // if compression fails, send error response
        Response response = {0};
        response.success = 0;
        strncpy(response.error_message, "Compression failed", sizeof(response.error_message));
        strncpy(response.request.filename, request.filename, sizeof(response.request.filename));
        strncpy(response.request.filecontent, request.filecontent, sizeof(response.request.filecontent));
        response.request.compressed_size = compressed_size;
        //strncpy(response.request.compressed_content, request.compressed_content, sizeof(response.request.compressed_content));
    }
    else {
        Response response = {0};
        response.success = 1;
        strncpy(response.error_message, "Compression successful", sizeof(response.error_message));
        strncpy(response.request.filename, request.filename, sizeof(response.request.filename));
        strncpy(response.request.filecontent, request.filecontent, sizeof(response.request.filecontent));
        response.request.compressed_size = compressed_size;
        strncpy(response.request.compressed_content, request.compressed_content, sizeof(response.request.compressed_content));
    }
}

void handle_signal(int signal) {
    // clean up resources
    munmap(shm_ptr, SHM_SIZE);
    shm_unlink(SHM_NAME);
    exit(0);
}

