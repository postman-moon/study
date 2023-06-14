#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pthread.h>

#define LL_ADD(item, list) do {     \
    item->prev = NULL;              \
    item->next = list;              \
    list = item;                    \
} while (0)

#define LL_REMOVE(item, list) do {                              \
    if (item->prev != NULL) item->prev->next = item->next;      \
    if (item->next != NULL) item->next->prev = item->prev;      \
    if (item == list) list = item->next;                        \
    item->prev = item->next = NULL;                             \
} while (0)

// 任务
struct NJOB {
    void (*job_function)(struct NJOB *job);     // 任务回调函数
    void *user_data;                            // 任务执行的参数

    struct NJOB *prev;
    struct NJOB *next;
};

// 执行
struct NWORKER {
    pthread_t thread;                           // 线程 id
    int terminate;                              // 终止标识
    struct NWORKQUEUE *workqueue;               // 池管理组件对象

    struct NWORKER *prev;
    struct NWORKER *next;
};

// 池管理组件
typedef struct NWORKQUEUE {
    struct NWORKER *workers;                    // 执行队列
    struct NJOB *waiting_jobs;                  // 任务队列

    pthread_mutex_t jobs_mtx;                   // 互斥锁，为任务加锁防止一个任务被两个线程执行等其它情况
    pthread_cond_t jobs_cond;                   // 条件变量，线程条件等待
} nWorkQueue;

typedef nWorkQueue nThreadPool;

static void * ntyWorkerThread(void *arg) {

    struct NWORKER *worker = (struct NWORKER*)arg;

    while (1) {

        pthread_mutex_lock(&worker->workqueue->jobs_mtx);

        while (worker->workqueue->waiting_jobs == NULL) {
            if (worker->terminate) break;
            pthread_cond_wait(&worker->workqueue->jobs_cond, &worker->workqueue->jobs_mtx);
        }

        if (worker->terminate) {
            pthread_mutex_unlock(&worker->workqueue->jobs_mtx);
            break;
        }

        struct NJOB *job = worker->workqueue->waiting_jobs;
        if (job != NULL) {
            LL_REMOVE(job, worker->workqueue->waiting_jobs);
        }

        pthread_mutex_unlock(&worker->workqueue->jobs_mtx);

        if (job == NULL) continue;

        job->job_function(job);
    }

    free(worker);
    pthread_exit(NULL);
}

/**
 * 线程池的创建
 * pool: 线程池的对象
 * numWorkers: 创建线程池的数量
*/
int ntyThreadPoolCreate (nThreadPool *pool, int numWorkers) {

    if (numWorkers < 1) numWorkers = 1;
    memset(pool, 0, sizeof(nThreadPool));

    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
    memcpy(&pool->jobs_cond, &blank_cond, sizeof(pool->jobs_cond));

    pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&pool->jobs_mtx, &blank_mutex, sizeof(pool->jobs_mtx));

    int i = 0;
    for (i = 0; i < numWorkers; i++) {
        struct NWORKER *worker = (struct NWORKER*)malloc(sizeof(struct NWORKER));
        if (worker == NULL) {
            perror("malloc");
            return -1;
        }

        memset(worker, 0, sizeof(struct NWORKER));
        worker->workqueue = pool;

        int ret = pthread_create(&worker->thread, NULL, ntyWorkerThread, (void *)worker);
        if (ret) {
            perror("pthread_create");
            free(worker);
            return -2;
        }

        LL_ADD(worker, worker->workqueue->workers); 
    }

    return 0;
}

/**
 * 线程池的销毁
 * pool：线程池的对象
*/
void ntyThreadPoolDestory(nThreadPool *pool) {

    struct NWORKER *worker = NULL;

    for (worker = pool->workers; worker != NULL; worker = worker->next) {
        worker->terminate = 1;
    }

    pthread_mutex_lock(&pool->jobs_mtx);

    pool->workers = NULL;
    pool->waiting_jobs = NULL;

    pthread_cond_broadcast(&pool->jobs_cond);

    pthread_mutex_unlock(&pool->jobs_mtx);

}

/**
 *  
*/
void ntyThreadPoolPushJob(nThreadPool *pool, struct NJOB *job) {

    pthread_mutex_lock(&pool->jobs_mtx);

    LL_ADD(job, pool->waiting_jobs);

    pthread_cond_signal(&pool->jobs_cond);

    pthread_mutex_unlock(&pool->jobs_mtx);

}


/************************** debug thread pool **************************/
#define KING_MAX_THREAD         80
#define KING_COUNTER_SIZE       1000

void king_counter(struct NJOB *job) {

    int index = *(int *)job->user_data;

    printf("index: %d, selfid: %lu\n", index, pthread_self());

    free(job->user_data);
    free(job);

}

int main(int argc, char const *argv[])
{
    nThreadPool pool;

    ntyThreadPoolCreate(&pool, KING_MAX_THREAD);

    int i = 0;
    for(i = 0; i < KING_COUNTER_SIZE; i++) {
        struct NJOB *job = (struct NJOB*)malloc(sizeof(struct NJOB));
        if (job == NULL) {
            perror("malloc");
            exit(1);
        }

        job->job_function = king_counter;
        job->user_data = malloc(sizeof(int));
        *(int *)job->user_data = i;

        ntyThreadPoolPushJob(&pool, job);
    }

    getchar();
    printf("\n");

    return 0;
}