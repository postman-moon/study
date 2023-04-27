

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bool.h>

struct list_node {
    int value;
    struct list_node *next;
};

// 单向链表如何找到倒数n个节点
struct list_node* find_list_node(struct list_node *head, int n) {

    struct list_node *fast = head;
    struct list_node *slow = head;

    for (int i = 0; i < n; i++) {
        fast = fast->next;
    }

    while (fast) {
        fast = fast->next;
        slow = slow->next;
    }

    return slow;
}

// 判断链表是否有环
bool hasCircles(struct list_node *head) {

    if (head == NULL) {
        return false;
    }

    struct list_node *fast = head->next;
    struct list_node *slow = head;

    while (fast) {

        if (fast == slow) {
            return true;
        }

        fast = fast->next;
        slow = slow->next;

        if (fast) {
            fast = fast->next;
        }

    }

    return fast;
}


// 判断两个链表是否有公共点
int foreach_list(struct list_node *head) {

    int length = 0;

    while (head) {
        length++;
        head = head->next;
    }

    return length;
}

bool isPublicNode(struct list_node *head1, struct list_node *head2) {

    struct list_node *longList, *shortList;
    int length1;
    int length2;
    int dif;

    length1 = foreach_list(head1);
    length2 = foreach_list(head2);

    if (length1 > length2) {
        longList = head1;
        shortList = head2;
        dif = length1 - lenght2;
    } else {
        longList = head2;
        shortList = head1;
        dif = length2 - length1;
    }

    while (dif--)
    {
        longList = longList->next;
    }

    while(longList) {

        if (longList == shortList) {
            return true;
        } else {
            longList = longList->next;
            shortList = shortList->next;
        }

    }
    
    return false;
}

// 链表反转
struct list_node* reverse_list(struct list_node *head) {

    struct list_node *prev = NULL;
    struct list_node *current = head;

    while (current) {
        
        struct list_node *node = current ->next;
        current->next = prev;
        prev = current;
        current = node;

    }

    return prev;
}

void show_list(struct list_node *head) {
    struct list_node *p = head;

    for (; p; p = p->next) printf("%d  ", p->value);

    printf("\n");
}

struct list_node *init_list(int n) {
    int i = 0;
    struct list_node *head, *p, *q;

    p = (struct list_node*)malloc(sizeof(struct list_node));
    head = p;
    for (i = 1;i < n;i ++) {
        q = (struct list_node*)malloc(sizeof(struct list_node));
        q->value = 2 * i;
        p->next = q;
        p = q;
    }

    p->next = NULL;

    return head;
}

int main() {

    struct list_node *head = init_list(10);
    show_list(head);

#if 0
    head = reverse_list(head);
    //inversion_node(head);

    show_list(head);
#elif 1
    struct list_node *node =  find_list_node(head, 8);

    printf("the last %d value is %d\n", 8, node->value);

#endif

    return 0;

}



