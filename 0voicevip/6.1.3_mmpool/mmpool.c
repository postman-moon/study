#include <stdio.h>
#include <stdlib.h>

#define MP_ALIGNMENT        32
#define PAGE_SIZE           4096

struct mp_large_s {
    struct mp_large_s *next;
    void *alloc;
};

struct mp_node_s {
    unsigned char *end;
    unsigned char *last;
    int failed;

    struct mp_node_s *next;
};

struct mp_pool_s {
    struct mp_large_s *large;

    struct mp_node_s *head;
    struct mp_node_s *current;
};

// 1 内存池的创建
struct mp_pool_s *mp_create(int size) {

    struct mp_pool_s *p;

    int ret = posix_memalign(&p, MP_ALIGNMENT, sizeof(struct mp_pool_s) + size);
    if(ret) {
        return NULL;
    }

    p->large = NULL;
    // p->current = p->head = p + 1;
    p->current = p->head = (unsigned char *)p + sizeof(struct mp_pool_s);
    p->head->last = (unsigned char *)p + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s);
    p->head->end = p->head->last + size - sizeof(struct mp_node_s);

    return p;

}

void mp_destory(struct mp_pool_s *pool) {

}

void *mp_alloc_large(struct mp_pool_s *pool, int size) {

    unsigned char *m;

    int ret = posix_memalign(&m, MP_ALIGNMENT, size);
    if (ret) return NULL;

    struct mp_large_s *large;
    for (large = pool->large; large; large = large->next) {
        
        if (large->alloc == NULL) {
            large->alloc = m;
            return m;
        }

    }

    large = mp_alloc(pool, sizeof(struct mp_large_s));

    large->alloc = m;
    large->next = pool->large;

    return m; 
}

void *mp_alloc_block(struct mp_pool_s *pool, int size) {

    unsigned char *m;

    int ret = posix_memalign(&m, MP_ALIGNMENT, PAGE_SIZE);
    if (ret) return NULL;

    struct mp_node_s *new_node = (struct mp_node_s*)m;
    new_node->end = m + PAGE_SIZE;
    new_node->next = NULL;
    new_node->last = m + sizeof(struct mp_node_s);

    struct mp_node_s *current = pool->current;


    struct mp_node_s *p = NULL;
    for (p = current; p->next; p = p->next) {
        
        if (p->failed ++ > 4) {
            current = p->next;
        }

    }

    p->next = new_node;
    pool->current = current ? current : new_node;

    return m;

}

void *mp_alloc(struct mp_pool_s *pool, int size) {

    if (size <= 0) return NULL;

    if (size > PAGE_SIZE - sizeof(struct mp_node_s)) { // large

        return mp_alloc_large(pool, size);

    } else {

        struct mp_node_s *p = pool->current;

        unsigned char *m = p->last;
        if (p->end - m >= size) {
            p->last = m + size;
            return m;
        } else {
            return mp_alloc_block(pool, size);
        }

    }

}

void mp_free(struct mp_pool_s *pool, void *p) {

}