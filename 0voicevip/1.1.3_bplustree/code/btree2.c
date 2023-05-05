#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int KEY_VALUE;

#define DEGREE      3

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

void btree_create(btree *T, int t) {

    T->degree = t;

    btree_node *x = btree_create_node(t, 1);
    T->root = x;

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
    if (y->leaf == 0) {
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

        x->keys[i + 1] = k;
        x->num += 1;

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

void btree_traverse(btree_node *x) {

    int i = 0;

    for (i = 0; i < x->num; i++) {
        if (x->leaf == 0) {
            btree_traverse(x->childrens[i]);
        }

        printf("%C ", x->keys[i]);
    }

    if (x->leaf == 0) {
        btree_traverse(x->childrens[i]);
    }

}

void btree_print(btree *T, btree_node *node, int layer) {

    btree_node *p = node;
    int i;

    if (p) {
        printf("\nlayer = %d keynum = %d is_leaf = %d\n", layer, p->num, p->leaf);
        for (i = 0; i < node->num; i++) {
            printf("%c ", p->keys[i]);
            printf("\n");

            #if 0
                printf("%p\n", p);
                for (i = 0; i <= 2 * T->degree; i++) {
                    printf("%p ", p->childrens[i]);
                }
                printf("\n");
            #endif

            layer++;
            for (i = 0; i <= p->num; i++) {
                if (p->childrens[i]) {
                    btree_print(T, p->childrens[i], layer);
                }

            }
        }
    } else {
        printf("the tree is empty\n");
    }

}

int btree_bin_search(btree_node *node, int low, int high, KEY_VALUE key) {

    int mid;

    if (low > high || low < 0 || high < 0) {
        return -1;
    }

    while (low <= high) {
        mid = (low + high) / 2;
        if (key > node->keys[mid]) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    return low;
}

void btree_merge(btree *T, btree_node *node, int idx) {

    btree_node *left = node->childrens[idx];
    btree_node *right = node->childrens[idx + 1];

    int i = 0;

    left->keys[T->degree - 1] = node->keys[idx];
    for (i = 0; i < T->degree - 1; i++) {
        left->keys[T->degree + i] = right->keys[i];
    }

    if (!left->leaf) {
        for (i = 0; i < T->degree; i++) {
            left->childrens[T->degree + i] = right->childrens[i];
        }
    }
    left->num += T->degree;

    btree_destroy_node(right);

    for (i = idx + 1; i < node->num; i++) {
        node->keys[i - 1] = node->keys[i];
        node->childrens[i]= node->childrens[i + 1];
    }
    node->childrens[i + 1] = NULL;
    node->num -= 1;

    if (node->num == 0) {
        T->root = left;
        btree_destroy_node(node);
    }

}

void btree_delete_key(btree *T, btree_node *node, KEY_VALUE key) {

    if (node == NULL) return ;

    int idx = 0, i;
    // 不管是哪个节点，从0开始找第一个大于它的值，再去找他的子树
    while (idx < node->num && key > node->keys[idx]) idx ++;

    if (idx < node->num && key == node->keys[idx]) {

        if (node->leaf) { // 是叶子节点

            for (i = idx; i < node->num-1; i++) {
                node->keys[i] = node->keys[i + 1];
            }
            node->keys[node->num - 1] = 0;
            node->num --;

            if (node->num == 0) { // root
                free(node);
                T->root = NULL;
            }

        } else if (node->childrens[idx]->num > T->degree - 1) { // 前面节点大于 degree - 1

            btree_node *left = node->childrens[idx];
            node->keys[idx] = left->keys[left->num - 1];
            btree_delete_key(T, left, left->keys[left->num - 1]);

        } else if (node->childrens[idx + 1]->num > T->degree -1) { // 后面节点大于 degree - 1

            btree_node *right = node->childrens[idx + 1];
            node->keys[idx] = right->keys[0];

            btree_delete_key(T, right, right->keys[0]);

        } else { // 前面节点等于 degree - 1，后面节点等于 degree - 1

            btree_merge(T, node, idx);
            btree_delete_key(T, node->childrens[idx], key);

        }

	} else {

		btree_node *child = node->childrens[idx];
		if (child == NULL) {
			printf("Cannot del key = %d\n", key);
			return ;
		}

		if (child->num == T->degree - 1) {

			btree_node *left = NULL;
			btree_node *right = NULL;
			if (idx - 1 >= 0)
				left = node->childrens[idx-1];
			if (idx + 1 <= node->num) 
				right = node->childrens[idx+1];

			if ((left && left->num >= T->degree) ||
				(right && right->num >= T->degree)) {

				int richR = 0;
				if (right) richR = 1;
				if (left && right) richR = (right->num > left->num) ? 1 : 0;

				if (right && right->num >= T->degree && richR) { //borrow from next
					child->keys[child->num] = node->keys[idx];
					child->childrens[child->num+1] = right->childrens[0];
					child->num ++;

					node->keys[idx] = right->keys[0];
					for (i = 0;i < right->num - 1;i ++) {
						right->keys[i] = right->keys[i+1];
						right->childrens[i] = right->childrens[i+1];
					}

					right->keys[right->num-1] = 0;
					right->childrens[right->num-1] = right->childrens[right->num];
					right->childrens[right->num] = NULL;
					right->num --;
					
				} else { //borrow from prev

					for (i = child->num;i > 0;i --) {
						child->keys[i] = child->keys[i-1];
						child->childrens[i+1] = child->childrens[i];
					}

					child->childrens[1] = child->childrens[0];
					child->childrens[0] = left->childrens[left->num];
					child->keys[0] = node->keys[idx-1];
					
					child->num ++;

					node->keys[idx-1] = left->keys[left->num-1];
					left->keys[left->num-1] = 0;
					left->childrens[left->num] = NULL;
					left->num --;
				}

			} else if ((!left || (left->num == T->degree - 1))
				&& (!right || (right->num == T->degree - 1))) {

				if (left && left->num == T->degree - 1) {
					btree_merge(T, node, idx-1);					
					child = left;
				} else if (right && right->num == T->degree - 1) {
					btree_merge(T, node, idx);
				}
			}
		}

		btree_delete_key(T, child, key);
	}
}

int btree_delete(btree *T, KEY_VALUE key) {

    if (!T->root) {
        return -1;
    }

    btree_delete_key(T, T->root, key);
    return 0;

}

int main() {
	btree T = {0};

	btree_create(&T, 3);
	srand(48);

	int i = 0;
	char key[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	for (i = 0;i < 26;i ++) {
		//key[i] = rand() % 1000;
		printf("%c ", key[i]);
		btree_insert(&T, key[i]);
	}

	btree_print(&T, T.root, 0);

	for (i = 0;i < 26;i ++) {
		printf("\n---------------------------------\n");
		btree_delete(&T, key[25-i]);
		//btree_traverse(T.root);
		btree_print(&T, T.root, 0);
	}
	
}