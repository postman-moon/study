#include <stdio.h>

#define DATA_ARRAY_LENGTH   12

/* 
第一个参数：样本的集合
第二个参数：样本的长度
 */
int shell_sort(int *data, int length) {

    int gap = 0;
    int i = 0, j = 0;

    for (gap = length / 2; gap >= 1; gap /= 2) { // 分组的次数

        for (i = gap; i < length; i++) { // 遍历每一组

            int temp = data[i];
            for (j = i - gap; j >= 0 && temp < data[j]; j -= gap) { // 组内排序

                printf("gap = %d, i = %d, j = %d \n", gap, i, j);

                data[j + gap] = data[j];

            }
            data[j + gap] = temp;

        }

    }

    return 0;
}

// 归并排序
int merge(int *data, int *temp, int start, int middle, int end) {

    int i = start, j = middle + 1, k = start;
    
    while (i <= middle && j <= end) { // [a]  [b] 将两个小的数组变有序

        if (data[i] > data[j]) {
            temp[k++] = data[j++]; // 取两只比较小的放进中间数组
        } else {
            temp[k++] = data[i++];
        }

    }

    while (i <= middle) {  // 当 i 或者 j 中有一组比较完，剩下的需要进行比较
        temp[k++] = data[i++];
    }

    while (j <= end) {
        temp[k++] = data[j++];
    }

    for (i = start; i <= end; i++) {
        data[i] = temp[i];
    }

    return 0;
}

int merge_sort(int *data, int *temp, int start, int end) {

    int middle;
    if (start < end) {
        middle = start + (end - start) / 2;
        merge_sort(data, temp, start, middle);
        merge_sort(data, temp, middle + 1, end);
        merge(data, temp, start, middle, end);
    }

    return 0;
}

int sort(int *data, int left, int right) { // 每一次递归，每调用一次，确定一个值的正确位置

    if (left >= right) return 0;

    int i = left;
    int j = right;
    int key = data[left]; // 哨兵

    while (i < j) {

        // 从后往前找到第一个比哨兵小的
        while (i < j && key < data[j]) {
            j--;
        }
        data[i] = data[j];

        // 从前往后找到第一个比哨兵大的
        while (i < j && key >= data[i]) {
            i++;
        }
        data[j] = data[i];
    }
    // i == j
    data[i] = key;

    // 使用递归
    sort(data, left, i - 1);
    sort(data, i + 1, right);

}

int quick_sort(int *data, int length) { // 快排入口

    sort(data, 0, length - 1);
    return 0;

}

int main(int argc, char const *argv[])
{
    int data[DATA_ARRAY_LENGTH] = {23, 64, 24, 12, 9, 16, 53, 57, 71, 79, 87, 97};

#if 0
    shell_sort(data, DATA_ARRAY_LENGTH);
#elif 0
    int temp[DATA_ARRAY_LENGTH] = {0};
    merge_sort(data, temp, 0, DATA_ARRAY_LENGTH - 1);
#elif 1
    quick_sort(data, DATA_ARRAY_LENGTH);

#endif

    for (int i = 0; i < DATA_ARRAY_LENGTH; i++) {
        printf("%4d", data[i]);
    }
    printf("\n");

    return 0;
}
