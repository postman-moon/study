#include <stdio.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/mman.h>

#define THREAD_SIZE     10

int count = 0;
pthread_mutex_t mutex;
pthread_spinlock_t spinlock;
pthread_rwlock_t rwlock;

int inc(int *value, int add) {

    int old;

    __asm__ volatile (
        "lock; xaddl %2, %1;"
        : "=a" (old)
        : "m" (*value), "a" (add)
        : "cc", "memory"
    );

    return old;

}

void *thread_callback(void *arg) {

    int *pcount = (int *)arg;

    int i = 0;

    while (i++ < 10000) {

#if 0
        (*pcount) ++;
#elif 0

    // 互斥锁
    pthread_mutex_lock(&mutex);
    (*pcount) ++;
    pthread_mutex_unlock(&mutex);

#elif 0

    if (0 != pthread_mutex_trylock(&mutex)) {
        i --;
        continue; 
    }
    (*pcount) ++;
    pthread_mutex_unlock(&mutex);

#elif 0

    // 自旋锁
    pthread_spin_lock(&spinlock);
    (*pcount) ++;
    pthread_spin_unlock(&spinlock);

#elif 0

    // 读写锁
    pthread_rwlock_wrlock(&rwlock);
    (*pcount) ++;
    pthread_rwlock_unlock(&rwlock);

#elif 1

    // 原子操作
    inc(pcount, 1);

#endif

        usleep(1);

    }

    return NULL;

}

int main(int argc, char const *argv[])
{
    
#if 0 
    // 多线程
    pthread_t threadid[THREAD_SIZE] = {0};

    int i = 0;

    pthread_mutex_init(&mutex, NULL);
    pthread_spin_init(&spinlock, PTHREAD_PROCESS_SHARED);
    pthread_rwlock_init(&rwlock, NULL);

    for (i = 0; i < THREAD_SIZE; i++) {

        int ret = pthread_create(&threadid[i], NULL, thread_callback, &count);
        if (ret) {
            perror("pthread_create");
            break;
        }

    }

    for (i = 0; i < 1000; i++) {

        pthread_rwlock_rdlock(&rwlock);
        printf("count --> %d\n", count);
        pthread_rwlock_unlock(&rwlock);

        sleep(1);

    }
#else

    // 多进程
    int i = 0;
    pid_t pid;

    int *pcount = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);

    for (i = 0; i < THREAD_SIZE; i++) {

        pid = fork();

        if (pid <= 0) {
            usleep(1);
            break;
        }

    }

    if (pid > 0) {

        for (i = 0; i < 100; i++) {
            printf("count --> %d\n", *pcount);
            sleep(1);
        }

    } else {

        int i = 0;
        while (i++ < 1000) {

#if 0

            (*pcount) ++;

#elif 1

            inc(pcount, 1);

#endif
            usleep(1);
        }

    }

#endif

    return 0;
}