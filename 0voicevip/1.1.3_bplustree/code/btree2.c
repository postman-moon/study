#include <stdio.h>


typedef int KEY_VALUE;

#define DEGREE      4

typedef struct _btree_node {

    KEY_VALUE *keys; // 代表这个节点里有多少个数
    struct _btree_node **childrens; // 指向下一层的子树

    int num; // 当前节点里有多少个节点
    int leaf; // 是否为叶子节点   yes: 1  no: 0

} btree_node;

typedef struct _btree {

    btree_node *root; // 根节点
    int degree; // 阶数

} btree;

btree_node * btree_create_node(int t, int leaf) {

    btree_node *node = (btree_node *)calloc(1, sizeof(btree_node));
    if (node == NULL) {
        printf("btree_create_node ----> calloc failed\n");
        return NULL;
    }

    /*
        memset()
        calloc = malloc + memset
    */

    node->leaf = leaf;
    node->keys = (KEY_VALUE *)calloc(1, (2 * t - 1) * sizeof(KEY_VALUE));
    if (node->keys == NULL) {
        free(node);
        return NULL;
    }

    node->childrens = (btree_node **)calloc(1, (2 * t) * sizeof(btree_node *));
    if (node->childrens == NULL) {
        free(node->keys);
        free(node);
        return NULL;
    }
    node->num = 0;

    return node;

}

void btree_destroy_node(btree_node *node) {

    if (node == NULL) return ;

    if (node->childrens) free(node->childrens);
    if (node->keys) free(node->keys);
    free(node);

}

void btree_split_child(btree *T, btree_node *x, int i) {

    int degree = T->degree;

    btree_node *y = x->childrens[i];
    btree_node *z = btree_create_node(degree, y->leaf);

    int j = 0;
    for (j = 0; j < degree - 1; j++) {
        z->keys[j] = y->keys[degree + j];
    }

    // 如果不是叶子节点时
    if (x->leaf == 0) {
        for (j = 0; j < degree; j++) {
            z->childrens[j] = y->childrens[j + degree];
        }
    }

    /*
        这之前是往z里面加值
    */

    z->num = degree - 1;
    y->num = degree - 1;

    for (j = x->num; j >= i + 1; j--) {
        x->childrens[j + 1] = x->childrens[j];
    }

    x->childrens[i + 1] = z;
    for (j = x->num - 1; j >= i; j--) {
        x->keys[j + 1] = x->keys[j];
    }
    x->keys[i] = y->keys[degree - 1];
    x->num++;

}

void btree_insert_nonfull(btree *T, btree_node *x, KEY_VALUE k) {

    int i = x->num - 1;
    if (x->leaf == 1) {

        while (i >= 0 && x->keys[i] > k) {
            x->keys[i + 1] = x->keys[i];
            i--;
        }

    } else {
        
        while (i >= 0 && x->keys[i] > k) i--;

        if (x->childrens[i + 1]->num == (2 * T->degree - 1)) {
            btree_split_child(T, x, i + 1);

            if (k > x->keys[i + 1]) i++;
        }

        btree_insert_nonfull(T, x->childrens[i + 1], k);

    }

}

int btree_insert(btree *T, KEY_VALUE key) {

    btree_node *root = T->root;
    if (root->num != 2 * T->degree - 1) {
        btree_insert_nonfull(T, root, key);
    } else {

        btree_node *node = btree_create_node(T->degree, 0);
        T->root = node;

        node->childrens[0] = root;
        btree_split_child(T, node, 0);

        int i = 0;
        if (node->keys[0] < key) i++;
        btree_insert_nonfull(T, node->childrens[i], key);
    }

}

void btree_delete_key(btree *T, btree_node *node, KEY_VALUE key) {

    int idx = 0;
    // 不管是哪个节点，从0开始找第一个大于它的值，再去找他的子树
    while (idx < node->num && key > node->keys[idx]) idx ++;

    if (idx < node->num && key == node->keys[idx]) {

        if (node->leaf) { // 是叶子节点

        } else if (node->childrens[idx]->num > T->degree - 1) { // 前面节点大于 degree - 1

        } else if (node->childrens[idx + 1]->num > T->degree -1) { // 后面节点大于 degree - 1

        } else { // 前面节点等于 degree - 1，后面节点等于 degree - 1

        }

    }
}

int btree_delete(btree *T, KEY_VALUE key) {



}