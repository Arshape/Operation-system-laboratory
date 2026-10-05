#include <unistd.h>
#include <stdlib.h>

#define BUF_SIZE 4096


static void reverse(char *s, ssize_t len) {
    for (ssize_t i = 0, j = len - 1; i < j; ++i, --j) {
        char tmp = s[i];
        s[i]  = s[j];
        s[j]  = tmp;
    }
}

static int write_all(int fd, const char *buf, ssize_t len) {
    ssize_t written = 0;
    while (written < len) {
        ssize_t w = write(fd, buf + written, len - written);
        if (w < 0) return -1;
        written += w;
    }
    return 0;
}

int main(void) {
    char buf[BUF_SIZE];
    ssize_t bytes;
    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        ssize_t start = 0;
        for (ssize_t i = 0; i < bytes; ++i) {
            if (buf[i] == '\n') {
                reverse(buf + start, i - start);
                start = i + 1;
            }
        }
        if (start < bytes) {
            reverse(buf + start, bytes - start);
        }

        if (write_all(STDOUT_FILENO, buf, bytes) == -1) {
            const char msg[] = "reverse-filter: write error\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }

    if (bytes < 0) {
        const char msg[] = "reverse-filter: read error\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    return 0;
}
