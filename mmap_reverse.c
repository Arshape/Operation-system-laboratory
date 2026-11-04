#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>

#define BUF_SIZE 4096

typedef struct {
    size_t len;
    char   data[BUF_SIZE];
} shm_buffer_t;

static void reverse(char *s, size_t len) {
    for (size_t i = 0, j = len - 1; i < j; ++i, --j) {
        char tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
    }
}

int main(int argc, char **argv) {
    if (argc < 4) {
        const char msg[] = "usage: reverse <shm_name> <sem_to> <sem_from>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return EXIT_FAILURE;
    }

    const char *shm_name     = argv[1];
    const char *sem_to_name  = argv[2];
    const char *sem_from_name= argv[3];

    int fd = shm_open(shm_name, O_RDWR, 0);
    if (fd == -1) {
        const char msg[] = "reverse: failed to open SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return EXIT_FAILURE;
    }

    shm_buffer_t *buf = mmap(NULL, sizeof(shm_buffer_t),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED, fd, 0);
    if (buf == MAP_FAILED) {
        const char msg[] = "reverse: failed to map SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return EXIT_FAILURE;
    }
    close(fd);

    sem_t *to   = sem_open(sem_to_name,   0);
    sem_t *from = sem_open(sem_from_name, 0);
    if (to == SEM_FAILED || from == SEM_FAILED) {
        const char msg[] = "reverse: failed to open semaphore\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        munmap(buf, sizeof(shm_buffer_t));
        return EXIT_FAILURE;
    }

    for (;;) {
        sem_wait(to);

        if (buf->len == 0) {
            sem_post(from);
            break;
        }
        size_t start = 0;
        for (size_t i = 0; i < buf->len; ++i) {
            if (buf->data[i] == '\n') {
                reverse(buf->data + start, i - start);
                start = i + 1;
            }
        }
        if (start < buf->len) {
            reverse(buf->data + start, buf->len - start);
        }
        sem_post(from);
    }
    sem_close(to);
    sem_close(from);
    munmap(buf, sizeof(shm_buffer_t));
    return 0;
}
