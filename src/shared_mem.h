#ifndef SHARED_MEM_H
#define SHARED_MEM_H

char * attach_mem_block(char *filename, int size, int id);
int dettach_mem_block(char *block);
int destroy_mem_block(char *filename, int id);
int get_sh_queue(char *filename, int id);
int destroy_sh_queue(char *filename, int id);
sem_t* create_semaphore(char *name, int value);
sem_t* attach_semaphore(char *name);

#define SHM_NAME "src/service.c"
#define SHM_SIZE 40960

#define WRITER_SEM_NAME "/writer"
#define READER_SEM_NAME "/reader"

// define request structure
typedef struct {
    long mtype;
    //char filename[256];
    size_t filesize;
    int unique_id;
    //int file_uploaded;
    //int compressed_size;
    //char compressed_content[SHM_SIZE];
} Request;

// define response structure
typedef struct {
    long mtype;
    //int success;
    int ready;
    int mem_size;
    size_t compressed_size;
    //char error_message[256];
    //Request request;
} Response;

#endif