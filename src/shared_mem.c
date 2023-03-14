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
#include "shared_mem.h"

static key_t get_key(char *filename, int id) {
    key_t key;
    
    key = ftok(filename, id);
    if (key == -1) return -1;
    return key;
}

static int get_shared_block(char *filename, int size, int id) {
    key_t key = get_key(filename, id);
    if (key == -1) return -1;

    // Create the shared memory segment
    return shmget(key, size, IPC_CREAT | 0666);
}

char * attach_mem_block(char *filename, int size, int id) {
    int shared_block_id = get_shared_block(filename, size, id);
    char *block_ptr;

    if (shared_block_id == -1) {
        perror("shmget");
        return NULL;
    }

    // Attach the shared memory segment to the process's address space
    block_ptr = shmat(shared_block_id, NULL, 0);
    if (block_ptr == (char *) -1) {
        perror("shmat");
        return NULL;
    }

    return block_ptr;
}

int dettach_mem_block(char *block) {
    // Detach the shared memory segment from the process' address space
    return shmdt(block);
}

int destroy_mem_block(char *filename, int id) {
    int shared_block_id = get_shared_block(filename, 0, id);
    if (shared_block_id == -1) {
        return -1;
    }

    return shmctl(shared_block_id, IPC_RMID, NULL);
}

int get_sh_queue(char *filename, int id) {
    key_t key = get_key(filename, id);
    if (key == -1) return -1;
    int msgq_id = msgget(key, IPC_CREAT | 0666);
    return msgq_id;
}

int destroy_sh_queue(char *filename, int id) {
    int msgq_id = get_sh_queue(filename, id);
    return msgctl(msgq_id, IPC_RMID, NULL);
}


