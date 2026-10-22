#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void fill_random(int *arr, size_t n, unsigned seed){
    srand(seed);
    for (size_t i = 0; i < n; ++i){
        arr[i] = rand();
    }
}

int is_sorted(const int *arr, size_t n){
    for (size_t i = 1; i < n; ++i)
        if (arr[i - 1] > arr[i])
            return 0;
    return 1;
}

double check_time(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}


void swap(int *a, int *b){
    int t = *a; 
    *a = *b; 
    *b = t;
}

int partitioning(int *arr, int left, int right){
    int mid = left + (right - left) / 2;
    if (arr[left] > arr[mid]){
        swap(&arr[left], &arr[mid]);
    }
    if (arr[left] > arr[right]){
        swap(&arr[left], &arr[right]);
    } 
    if (arr[mid] > arr[right]){
        swap(&arr[mid], &arr[right]);
    }

    int pivot = arr[mid];
    swap(&arr[mid], &arr[right - 1]);
    int i = left, j = right - 1;
    for (;;) {
        while (arr[++i] < pivot) {}
        while (arr[--j] > pivot) {}
        if (i >= j) break;
        swap(&arr[i], &arr[j]);
    }
    swap(&arr[i], &arr[right - 1]);
    return i;
}

void quicksort(int *arr, int left, int right){
    while (left < right) {
        if (right - left < 16) {
            for (int i = left + 1; i <= right; ++i) {
                int key = arr[i], j = i - 1;
                while (j >= left && arr[j] > key) { arr[j + 1] = arr[j]; --j; }
                arr[j + 1] = key;
            }
            return;
        }
        int p = partitioning(arr, left, right);
        if (p - left < right - p) {
            quicksort(arr, left, p - 1);
            left = p + 1;
        } else {
            quicksort(arr, p + 1, right);
            right = p - 1;
        }
    }
}

int main(int argc, char **argv){
    size_t n = 5000000;
    if (argc > 1) n = strtoull(argv[1], NULL, 10);

    int *arr = malloc(n * sizeof(int));
    if (arr == NULL) { 
        perror("malloc"); 
        return 1; 
    }
    fill_random(arr, n, 13);

    double t0 = check_time();
    quicksort(arr, 0, (int)n - 1);
    double t1 = check_time();

    printf("[последовательная версия] n=%zu time=%.3f ms sorted=%s\n", n, t1 - t0, is_sorted(arr, n) ? "yes" : "NO");
    free(arr);
    return 0;
}
