#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_SIZE     10

int count = 0;
pthread_mutex_t mutex;
pthread_spinlock_t spinlock;

void *thread_callback(void *arg) {

    // 用指针是对 count 变量修改
    int *pcount = (int *)arg;
    int i = 0;

    while (i++ < 100000) {

#if 0

        (*pcount) ++;

#elif 0

        pthread_mutex_lock(&mutex);

        (*pcount) ++;

        pthread_mutex_unlock(&mutex);

#elif 1

        pthread_spin_lock(&spinlock);

        (*pcount) ++;

        pthread_spin_unlock(&spinlock);
    
#endif


        usleep(1);
    }

    return NULL;
}

int main(int argc, char const *argv[])
{

    pthread_t threadid[THREAD_SIZE] = {0};
    int i = 0;

    pthread_mutex_init(&mutex, NULL); 
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_SHARED);

    // 开启十个子线程
    for (i = 0; i < THREAD_SIZE; i++) {

        int ret = pthread_create(&threadid[i], NULL, thread_callback, &count);
        if (ret) {
            break;
        }

    }

    // 主线程做统计
    for (i = 0; i < 100; i++) {

        printf("count --> %d\n", count);
        sleep(1);

    }

    pthread_mutex_destroy(&mutex);
    pthread_spin_destroy(&spinlock);
    
    return 0;
}
