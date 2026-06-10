#include "message_buffer.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

int shm_fd = -1;
void *memory_segment = NULL;

int init_buffer(MessageBuffer **buffer) {
    /* (1) create POSIX shared memory        */
    /*     using shm_open()                  */
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return -1;
    }

    /* (2) set size using ftruncate()        */
    if (ftruncate(shm_fd, sizeof(MessageBuffer)) == -1) {
        perror("ftruncate failed");
        return -1;
    }

    /* (3) map memory using mmap()           */
    memory_segment = mmap(NULL, sizeof(MessageBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memory_segment == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    /* (4) initialize MessageBuffer fields   */
    *buffer = (MessageBuffer *)memory_segment;
    (*buffer)->is_empty = 1;

    printf("init buffer\n");
    return 0;
}

int attach_buffer(MessageBuffer **buffer) {
    
    /* (1) open POSIX shared memory          */
    /*     using shm_open()                  */
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return -1;
    }
    
    /* (2) map memory using mmap()           */
    memory_segment = mmap(NULL, sizeof(MessageBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memory_segment == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }

    /* (3) assign mapped address to *buffer  */
    *buffer = (MessageBuffer *)memory_segment;

    

    printf("attach buffer\n");
    printf("\n");
    return 0;
}

int detach_buffer(void) {
    if (munmap(memory_segment, sizeof(MessageBuffer)) == -1) {
        printf("munmap error!\n\n");
        return -1;
    }

    if (shm_fd != -1) close(shm_fd);

    printf("detach buffer\n\n");
    return 0;
}

int destroy_buffer(void) {
    if(shm_unlink(SHM_NAME) == -1) {
        printf("shm_unlink error!\n\n");
        return -1;
    }

    printf("destroy shared_memory\n\n");
    return 0;
}

int produce(MessageBuffer **buffer, int sender_id, int data, int account_id) {

    
    /* TODO 3 : produce message              */

	while ((*buffer)->is_empty == 0) {
    }

    (*buffer)->message.sender_id = sender_id;
    (*buffer)->message.data = data;
    (*buffer)->account_id = account_id;

    (*buffer)->is_empty = 0;

    printf("produce message\n");

    return 0;
}

int consume(MessageBuffer **buffer, Message **message) {

    /* TODO 4 : consume message              */
    if ((*buffer)->is_empty == 1) {
        return -1;
    }

    *message = &((*buffer)->message);

    (*buffer)->is_empty = 1;
    
    return 0;
}
