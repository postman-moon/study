#include <stdio.h>

#define DATA_ARRAY_LENGTH   12

// 希尔排序
int shell_sort(int *data, int length) {

    int gap = 0;
    int i = 0, j = 0;

    for (gap = length / 2; gap >= 1; gap = gap / 2) {

        for (i = gap; i < length; i++) {

            int temp = data[i];
            for (j = i - gap; j >= 0 && temp < data[j]; j = j - gap) {
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
    while(i <= middle && j <= end) {

        if (data[i] > data[j]) {
            temp[k++] = data[j++];
        } else {
            temp[k++] = data[i++];
        }

    }

    while(i <= middle) {
        temp[k++] = data[i++];
    }

    while (j <= end) {
        temp[k++] = data[j++];
    }

    for (i = start; i <= end; i ++) {
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

// 快排
int quick(int *data, int left, int right) {

    if (left >= right) return -1;

    int i = left;
    int j = right;
    int key = data[left];

    while (i < j) {

        while (i < j && key < data[j]) {
            j--;
        }
        data[i] = data[j];

        while(i < j && key >= data[i]) {
            i++;
        }
        data[j] = data[i];

    }

    data[i] = key;

    quick(data, left, i - 1);
    quick(data, i + 1, right);

    return 0;
}

int quick_sort(int *data, int length) {

    quick(data, 0, length - 1);
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