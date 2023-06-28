#include <stdio.h>

#include <pthread.h>

// 任务
struct NJOB {
    void (*job_function)(struct NJOB *job);
    void *user_data;

    struct NJOB *prev;
    struct NJOB *next;
};

// 执行
struct NWORKER {
    pthread_t threadId;
    int terminate;
    struct NWORKQUEUE *workqueue;

    struct NWORKER *prev;
    struct NWORKER *next;
};

struct NWORKQUEUE {

};