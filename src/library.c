#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define MAX_FILENAME_LEN 256
#define MAX_CONTENT_LEN 4096
#define MAX_RESPONSE_LEN 4096

typedef struct {
    char filename[MAX_FILENAME_LEN];
    int content_len;
    char content[MAX_CONTENT_LEN];
} tinyfile_request;

typedef struct {
    int success;
    int compressed_len;
    char compressed_content[MAX_CONTENT_LEN];
} tinyfile_response;

typedef struct {
    int sync;
    tinyfile_request request;
    tinyfile_response response;
    int completed;
} tinyfile_call;

typedef struct {
    int queue_size;
    int next_write;
    int next_read;
    tinyfile_call calls[];
} tinyfile_queue;

static int fd;
static tinyfile_queue* queue;
static char* shared_mem;
static size_t shared_mem_size;

static int create_shared_mem() {
    fd = shm_open("/tinyfile", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open");
        return -1;
    }

    shared_mem_size = sizeof(tinyfile_queue) + MAX_FILENAME_LEN + MAX_CONTENT_LEN + MAX_RESPONSE_LEN;
    if (ftruncate(fd, shared_mem_size) == -1) {
        perror("ftruncate");
        return -1;
    }

    shared_mem = mmap(NULL, shared_mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("mmap");
        return -1;
    }

    queue = (tinyfile_queue*) shared_mem;
    queue->queue_size = (shared_mem_size - sizeof(tinyfile_queue)) / sizeof(tinyfile_call);
    queue->next_write = 0;
    queue->next_read = 0;

    return 0;
}

static void destroy_shared_mem() {
    if (shm_unlink("/tinyfile") == -1) {
        perror("shm_unlink");
    }

    if (munmap(shared_mem, shared_mem_size) == -1) {
        perror("munmap");
    }

    if (close(fd) == -1) {
        perror("close");
    }
}

static int enqueue_request(tinyfile_request* request, int sync) {
    int next_write = (queue->next_write + 1) % queue->queue_size;
    if (next_write == queue->next_read) {
        return -1; // queue is full
    }

    tinyfile_call* call = &queue->calls[queue->next_write];
    call->sync = sync;
    memcpy(&call->request, request, sizeof(tinyfile_request));
    call->completed = 0;

    queue->next_write = next_write;

    return 0;
}

static int dequeue_response(tinyfile_response* response) {
    if (queue->next_read == queue->next_write) {
        return -1; // queue is empty
    }

    tinyfile_call* call = &queue->calls[queue->next_read];
    if (!call->completed) {
        return 0; // call not yet completed
    }

    memcpy(response, &call->response, sizeof(tinyfile_response));

    queue->next_read = (queue->next_read + 1) % queue->queue_size;

    return 1;
}

static int wait_for_response() {
    while (1) {
        tinyfile_response
