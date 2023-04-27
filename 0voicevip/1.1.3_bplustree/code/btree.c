#include <stdio.h>

#include <assert.h>

#define M               6
#define DEGREE          (M/2)

typedef int KEY_VALUE;

struct btree_node {
    KEY_VALUE *keys;
    struct btree_node **childrens;

    int num;
    int leaf; // 是否为叶子节点
};

struct btree {
  
    struct btree_node *root;
    int t;
};

struct btree_node *btree_create_node(int t, int leaf) {

    struct btree_node *node = (struct btree_node *)calloc(1, sizeof(struct btree_node));
    if (NULL == node) {
        return NULL;
    }

    node->leaf = leaf;
    node->keys = (KEY_VALUE *)calloc(1, (2*t-1) * sizeof(KEY_VALUE));
    node->childrens = (struct btree_node **)calloc(1, (2 * t) * sizeof(struct btree_node));
    node->num = 0;

    return node;
}

void btree_destory_node(struct btree_node *node) {

    assert(node);

    free(node->childrens);
    free(node->keys);
    free(node);

}

void btree_create(struct btree *T, int t) {
    T->t = t;

    struct btree_node *x = btree_create_node(t, 1);
    T->root = x;

}

// 分裂
/*
    T：代表的是根，在哪个树上
    x：代表的是相当于图中的C
    i：代表的是C这个节点的第几个儿子
*/
void btree_split_child(struct btree *T, struct btree_node *x, int i) {
    int t = T->t;

    struct btree_node *y = x->childrens[i];
    struct btree_node *z = btree_create_node(t,  y->leaf);

    z->num = t - 1;

    // 创建一个新的节点 z，并将 y 的后 t - 1个 keys 和 子节点放入其中
    int j = 0;
    for (j = 0; j < t - 1; j++) {
        z->keys[j] = y->keys[j + t];
    }
    if (y->leaf == 0) {
        for (j = 0; j < t; j++) {
            z->childrens[j] = y->childrens[j + t];
        }
    }

    // 将y的keys和childrens数量更新为 t - 1
    y->num = t - 1;
    for (j = x->num; j >= i + 1; j --) {
        x->childrens[j + 1] = x->childrens[j];
    }

    x->childrens[i+1] = z;

    for (j = x->num-1; j >= i; j--) {
        x->keys[j + 1] = x->keys[j];
    }
    x->keys[i] = y->keys[t-1];
    x->num += 1;

}