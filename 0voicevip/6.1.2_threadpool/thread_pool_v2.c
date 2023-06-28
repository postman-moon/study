#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pthread.h>


// 链表的添加（头插法）
#define LL_ADD(item, list) do {             \
    item->prev = NULL;                      \
    item->next = list;                      \
    if (list != NULL) list->prev = item;    \
    list = item;                            \
} while (0)

// 链表的删除
#define LL_REMOVE(item, list) do {                              \
    if (item->prev != NULL) item->prev->next = item->next;      \
    if (item->next != NULL) item->next->prev = item->prev;      \
    if (item == list) list = item->next;                        \
    item->prev = item->next = NULL;                             \
} while (0)

// 任务
struct NJOB {
    void (*job_function) (struct NJOB *job);        // 任务回调函数
    void *user_data;                                // 任务执行的参数

    struct NJOB *prev;
    struct NJOB *next;
};

// 执行
struct NWORKER {
    pthread_t thread;                               // 线程 id
    struct NWORKQUEUE *workqueue;                   // 池管理组件对象，可以用来查看任务
    int terminate;                                  // 终止标识，用于判断工作是否结束，以便于终止

    struct NWORKER *prev;
    struct NWORKER *next;
};

// 池管理组件
struct NWORKQUEUE {
    struct NJOB     *waiting_jobs;                   // 任务队列
    struct NWORKER  *workers;                        // 执行队列

    pthread_mutex_t jobs_mtx;                        // 互斥锁
    pthread_cond_t  jobs_cond;                       // 条件变量
};


// 回调函数
static void *nThreadPoolCallback(void *arg) {

    // 接受 pthread_create 传来的参数
    struct NWORKER *worker = (struct NWORKER*)arg;

    while (1) { // 默认一直执行

        // 加锁
        pthread_mutex_lock(&worker->workqueue->jobs_mtx);

        while (worker->workqueue->waiting_jobs == NULL) {
            if (worker->terminate) break;
            // 如果每个任务就等待
            pthread_cond_wait(&worker->workqueue->jobs_cond, &worker->workqueue->jobs_mtx);
        }

        if (worker->terminate) {
            // 解锁
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


// 线程池的创建
int nThreadPoolCreate(struct NWORKQUEUE *pool, int numWorkers) {

    if (pool == NULL) return -1;
    // 如果任务数量小于1，就默认为1
    if (numWorkers < 1) {
        numWorkers = 1;
    }
    memset(pool, 0, sizeof(struct NWORKQUEUE));

    // 创建互斥量 cond，静态全局
    pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
    memcpy(&pool->jobs_cond, &blank_cond, sizeof(pool->jobs_cond));

    // 创建互斥锁mutex，静态全局
    pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
    memcpy(&pool->jobs_mtx, &blank_mutex, sizeof(pool->jobs_mtx));

    int i = 0;
    for (i = 0; i < numWorkers; i++) {
        struct NWORKER *worker = (struct NWORKER*)malloc(sizeof(struct NWORKER));
        if (worker == NULL) {
            perror("malloc");
            return -2;
        }
        memset(worker, 0, sizeof(struct NWORKER));
        worker->workqueue = pool;

        int ret = pthread_create(&worker->thread, NULL, nThreadPoolCallback, worker);
        if (ret) {
            perror("pthread_create");
            free(worker);
            return -3;
        }

        LL_ADD(worker, pool->workers);
    }

    return 0;

}

// 线程池的销毁
void nThreadPoolDestory(struct NWORKQUEUE *pool) {

    struct NWORKER *worker = NULL;

    for (worker = pool->workers; worker != NULL; worker = worker->next) {
        worker->terminate = 1;
    }

    pthread_mutex_lock(&pool->jobs_mtx);

    pool->workers = NULL; // 置空
    pool->waiting_jobs = NULL; // 置空

    // 唤醒所有线程
    pthread_cond_broadcast(&pool->jobs_cond);

    pthread_mutex_unlock(&pool->jobs_mtx);

}

// 加入线程池
void nThreadPoolPushJob(struct NWORKQUEUE *pool, struct NJOB *job) {

    pthread_mutex_lock(&pool->jobs_mtx);

    LL_ADD(job, pool->waiting_jobs);

    // 唤醒一个线程
    pthread_cond_signal(&pool->jobs_cond);

    pthread_mutex_unlock(&pool->jobs_mtx);

}


/************************** debug thread pool **************************/
#define THREADPOOL_INIT_COUNT       20
#define TASK_INIT_SIZE              1000

void task_entry(struct NJOB *job) {

    int index  = *(int *)job->user_data;

    printf("index: %d, selfid: %lu\n", index, pthread_self());

    free(job->user_data);
    free(job);

}

int main(int argc, char const *argv[])
{
    
    struct NWORKQUEUE pool = {0};

    nThreadPoolCreate(&pool, THREADPOOL_INIT_COUNT);

    int i = 0;
    for (i = 0; i < TASK_INIT_SIZE; i++) {

        struct NJOB *job = (struct NJOB*)malloc(sizeof(struct NJOB));
        if (job == NULL) {
            perror("malloc");
            exit(1);
        }
        memset(job, 0, sizeof(struct NJOB));

        job->job_function = task_entry;
        job->user_data = malloc(sizeof(int));
        *(int *)job->user_data = i;

        nThreadPoolPushJob(&pool, job);

    }

    // 防止主线程提前结束任务
    getchar();

    return 0;
}
