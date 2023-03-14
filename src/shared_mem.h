#ifndef SHARED_MEM_H
#define SHARED_MEM_H

char * attach_mem_block(char *filename, int size, int id);
int dettach_mem_block(char *block);
int destroy_mem_block(char *filename, int id);
int get_sh_queue(char *filename, int id);
int destroy_sh_queue(char *filename, int id);

#define SHM_NAME "src/service.c"
#define SHM_SIZE 40960

// define request structure
typedef struct {
    long mtype;
    char filename[256];
    int filesize;
    int unique_id;
    int file_uploaded;
    //int compressed_size;
    //char compressed_content[SHM_SIZE];
} Request;

// define response structure
typedef struct {
    long mtype;
    int success;
    int ready;
    int mem_size;
    char error_message[256];
    Request request;
} Response;

#endif