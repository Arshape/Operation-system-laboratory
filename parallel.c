#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <pthread.h>

void fill_random(int *arr, size_t n, unsigned seed){
    srand(seed);
    for (size_t i = 0; i < n; ++i){
        arr[i] = rand();
    }
}

int is_sorted(const int *arr, size_t n){
    for (size_t i = 1; i < n; ++i)
        if (arr[i - 1] > arr[i]) return 0;
    return 1;
}

double check_time(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

typedef struct {
    int value;   
    pthread_mutex_t mtx;     
} MySem;

void mysem_init(MySem *s, int initial_value){
    s->value = initial_value;
    pthread_mutex_init(&s->mtx, NULL);
}

void mysem_destroy(MySem *s){
    pthread_mutex_destroy(&s->mtx);
}

int mysem_trywait(MySem *s){
    int ret = -1;
    pthread_mutex_lock(&s->mtx);
    if (s->value > 0) {
        s->value--;
        ret = 0;
    }
    pthread_mutex_unlock(&s->mtx);
    return ret;
}

void mysem_post(MySem *s){
    pthread_mutex_lock(&s->mtx);
    s->value++;
    pthread_mutex_unlock(&s->mtx);
}

void swap(int *a, int *b) { 
    int t = *a; 
    *a = *b; 
    *b = t; 
}

int partition(int *arr, int left, int right){
    int mid = left + (right - left) / 2;
    if (arr[left] > arr[mid])   swap(&arr[left], &arr[mid]);
    if (arr[left] > arr[right]) swap(&arr[left], &arr[right]);
    if (arr[mid] > arr[right])  swap(&arr[mid], &arr[right]);
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
                while (j >= left && arr[j] > key){ 
                    arr[j + 1] = arr[j]; --j; 
                }
                arr[j + 1] = key;
            }
            return;
        }
        int p = partition(arr, left, right);
        if (p - left < right - p){ 
            quicksort(arr, left, p - 1); 
            left = p + 1; 
        }else{ 
            quicksort(arr, p + 1, right); 
            right = p - 1; 
        }
    }
}

typedef struct {
    int    *arr;
    int     left;
    int     right;
    MySem  *sem;
} Inner;

void *qs_run(void *arg){
    Inner *in = (Inner *)arg;
    int *arr   = in->arr;
    int  left  = in->left;
    int  right = in->right;
    MySem *sem = in->sem;

    while (left < right) {
        if (right - left < 16) {
            quicksort(arr, left, right);
            break;
        }

        int p = partition(arr, left, right);
        int spawned = 0;

        if ((right - p) >= 16 && mysem_trywait(sem) == 0) {
            Inner *r = malloc(sizeof(Inner));
            if (r) {
                *r = (Inner){arr, p + 1, right, sem};
                pthread_t tid;
                if (pthread_create(&tid, NULL, qs_run, r) == 0) {
                    spawned = 1;
                    Inner l = {arr, left, p - 1, sem};
                    qs_run(&l);
                    pthread_join(tid, NULL);
                    free(r);
                } else {
                    free(r);
                    mysem_post(sem);
                }
            } else {
                mysem_post(sem);
            }
        }

        if (spawned) {
            mysem_post(sem);
            return NULL;
        }
        quicksort(arr, left, p - 1);
        left = p + 1;
    }
    return NULL;
}

int main(int argc, char **argv){
    size_t n = 5000000;
    int max_threads = 4;
    int opt;

    while ((opt = getopt(argc, argv, "n:t:")) != -1) {
        switch (opt) {
        case 'n': n = strtoull(optarg, NULL, 10); break;
        case 't': max_threads = atoi(optarg);     break;
        }
    }

    if (max_threads < 1) {
        fprintf(stderr, "error: -t must be >= 1\n");
        return 1;
    }

    int *arr = malloc(n * sizeof(int));
    if (!arr) { perror("malloc"); return 1; }
    fill_random(arr, n, 13);

    MySem sem;
    mysem_init(&sem, max_threads);

    Inner root = { arr, 0, (int)n - 1, &sem };

    double t0 = check_time();
    qs_run(&root);
    double t1 = check_time();

    mysem_destroy(&sem);

    printf("[параллельная версия с числом потоков t=%d] n=%zu time=%.3f ms sorted=%s\n", 
           max_threads, n, t1 - t0, is_sorted(arr, n) ? "yes" : "NO");

    free(arr);
    return 0;
}
