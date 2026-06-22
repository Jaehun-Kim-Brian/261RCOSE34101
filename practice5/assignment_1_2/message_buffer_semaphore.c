#include "message_buffer_semaphore.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

int shm_fd = -1;
void *memory_segment = NULL;

sem_t *sem = SEM_FAILED;

void init_sem(void) {
    /* create/open POSIX named semaphore     */
    /* using sem_open() with initial value 1 */
    sem = sem_open(SEM_NAME, O_CREAT | O_RDWR, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open error");
    }
    
    printf("init semaphore : %s\n", SEM_NAME);
}

void destroy_sem(void) {
    /* close using sem_close() and remove    */
    /* named semaphore using sem_unlink()    */
    if (sem != SEM_FAILED) {
        sem_close(sem);
    }
    if (sem_unlink(SEM_NAME) == -1) {
        perror("sem_unlink error");
    }

}

void s_wait(void) {
    if (sem == SEM_FAILED) {
        sem = sem_open(SEM_NAME, 0);
        if (sem == SEM_FAILED) {
            printf("<s_wait> sem_open error!\n");
            return;
        }
    }

    if (sem_wait(sem) == -1) {
        printf("<s_wait> sem_wait error!\n");
	return;
    }
}

void s_quit(void) {
    if (sem == SEM_FAILED) {
        sem = sem_open(SEM_NAME, 0);
        if (sem == SEM_FAILED) {
            printf("<s_quit> sem_open error!\n");
            return;
        }
    }

    if (sem_post(sem) == -1) {
        printf("<s_quit> sem_post error!\n");
	return;
    }
}


int init_buffer(MessageBuffer **buffer) {
    /* (1) create POSIX shared memory        */
    /*     using shm_open()                  */
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) return -1;

    /* (2) set size using ftruncate()        */
    if (ftruncate(shm_fd, sizeof(MessageBuffer)) == -1) return -1;

    /* (3) map memory using mmap()           */
    memory_segment = mmap(NULL, sizeof(MessageBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memory_segment == MAP_FAILED) return -1;

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
    if (shm_fd == -1) return -1;

    /* (2) map memory using mmap()           */
    memory_segment = mmap(NULL, sizeof(MessageBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (memory_segment == MAP_FAILED) return -1;

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
    if (sem != SEM_FAILED) sem_close(sem);

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
    /* protect the shared buffer using       */
    /* s_wait() and s_quit()                 */
    /* write sender_id, data, and account_id */
    /* to the shared single-slot buffer      */
    int produced = 0;
    while (!produced) {
        s_wait(); 
        
        if ((*buffer)->is_empty == 1) {
            (*buffer)->message.sender_id = sender_id;
            (*buffer)->message.data = data;
            (*buffer)->account_id = account_id;
            (*buffer)->is_empty = 0; 
            produced = 1;            
        }
        
        s_quit(); 
    }
    printf("produce message\n");

    return 0;
}

int consume(MessageBuffer **buffer, Message **message) {
    
    /* protect the shared buffer using       */
    /* s_wait() and s_quit()                 */
    /* read the message from shared memory   */
    /* and update the buffer state           */
    s_wait(); 
    
    if ((*buffer)->is_empty == 1) {
        s_quit();
        return -1; 
    }

    *message = &((*buffer)->message);
    (*buffer)->is_empty = 1; 

    s_quit(); 
    
    return 0;
}

