#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void make_next(const char *pattern, int *next) {

    int q, k;
    int m = strlen(pattern);

    next[0] = 0; // 第一位为 0
    /* 
        q：后缀
        k：前缀
     */
    for (q = 1, k = 0; q < m; q++) {

        // 当开始不匹配时，利用已有的next开始回退以匹配的字符串，直到遇到相同的字符或者退回到k为0的位置
        while (k > 0 && pattern[q] != pattern[k]) {
            k = next[k - 1];
        }

        if (pattern[q] == pattern[k]) { // 如何前缀和后缀有相同的字符
            k ++;
        }

        next[q] = k;

    }

}

int kmp(const char *text, const char *pattern, int *next) {

    int n = strlen(text);
    int m = strlen(pattern);

    make_next(pattern, next);

    int i, q;
    for (i = 0, q = 0; i < n; i++) { // i --> text, q --> pattern

        // 先看后面的再看这算代码
#if 1
        while (q > 0 && pattern[q] != text[i]) {
            q = next[q - 1];
        }
#endif

        if (pattern[q] == text[i]) {
            q ++;
        }

        if (q == m) {
            return i - q + 1;
        }

    }

    return -1;
}

int main(int argc, char const *argv[])
{
    
    char *text = "abcabcabcabcabcd";
    char *pattern = "abcabcd";
    int next[20] = {0};

    int idx = kmp(text, pattern, next);

    for (int i = 0; i < 20; i++) {
        printf("%4d", next[i]);
    }
    printf("\n");

    printf("match pattern: %d\n", idx);

    return 0;
}
