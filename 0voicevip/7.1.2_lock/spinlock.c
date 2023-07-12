#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define PTHREAD_SIZE    10

int count = 0;
pthread_spinlock_t spinlock;

void *thread_callback(void *arg) {

    int *pcount = (int *)arg;
    int i = 0;

    while (i ++ < 10000) {

        pthread_spin_lock(&spinlock);

        (*pcount) ++;

        pthread_spin_unlock(&spinlock);

        usleep(1);
        
    }

    return NULL;
}

int main(int argc, char const *argv[])
{
    
    pthread_t threadid[PTHREAD_SIZE] = {0};
    int i = 0;
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_SHARED);

    for (i = 0; i < PTHREAD_SIZE; i++) {
        int ret = pthread_create(&threadid[i], NULL, thread_callback, &count);
        if(ret) {
            perror("pthread_create");
            break;
        }
    }

    for (i = 0; i < 100; i ++) {
        printf("count --> %d\n", count);
        sleep(1);
    }

    pthread_spin_destroy(&spinlock);

    return 0;
}
