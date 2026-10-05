#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

#define BUF_SIZE 4096
#define FILTER_NAME "reverse"

static int write_all(int fd, const char *buf, ssize_t len) {
    ssize_t written = 0;
    while (written < len) {
        ssize_t w = write(fd, buf + written, len - written);
        if (w < 0) return -1;
        written += w;
    }
    return 0;
}

static ssize_t read_line(int fd, char *buf, size_t max) {
    ssize_t total = 0;
    while (total < (ssize_t)max) {
        ssize_t r = read(fd, buf + total, 1);
        if (r < 0) return -1;
        if (r == 0) return total;
        ++total;
        if (buf[total - 1] == '\n') break;
    }
    return total;
}

static bool roll_80(void) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        unsigned seed = (unsigned)time(NULL) ^ (unsigned)getpid();
        return (seed % 100) < 80;
    }
    unsigned char byte;
    do {
        if (read(fd, &byte, 1) != 1) {
            close(fd);
            return true;
        }
    } while (byte >= 200);
    close(fd);
    return (byte % 100) < 80; 
}

int main(int argc, char **argv) {
    if (argc < 2) {
        const char msg[] = "usage: client path/to/reverse-filter [output_file]\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_SUCCESS);
    }

    const char *filterpath = argv[1];

    int out_fd = STDOUT_FILENO;
    if (argc >= 3) {
        out_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (out_fd == -1) {
            const char msg[] = "error: failed to open output file\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
    }

    int to_c1[2], to_c2[2], from_c1[2], from_c2[2];
    if (pipe(to_c1) == -1 || pipe(to_c2) == -1 ||
        pipe(from_c1) == -1 || pipe(from_c2) == -1) {
        const char msg[] = "error: failed to create pipe\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    pid_t c1 = fork();
    if (c1 == -1) {
        const char msg[] = "error: fork failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    if (c1 == 0) {
        close(to_c1[1]);
        close(to_c2[0]); close(to_c2[1]);
        close(from_c1[0]);
        close(from_c2[0]); close(from_c2[1]);
        if (out_fd != STDOUT_FILENO) close(out_fd);

        dup2(to_c1[0],   STDIN_FILENO);
        dup2(from_c1[1], STDOUT_FILENO);
        close(to_c1[0]);
        close(from_c1[1]);

        char *const args[] = { (char *)FILTER_NAME, NULL };
        execv(filterpath, args);

        const char msg[] = "error: exec failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    pid_t c2 = fork();
    if (c2 == -1) {
        const char msg[] = "error: fork failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    if (c2 == 0) {
        close(to_c1[0]); close(to_c1[1]);
        close(to_c2[1]);
        close(from_c1[0]); close(from_c1[1]);
        close(from_c2[0]);
        if (out_fd != STDOUT_FILENO) close(out_fd);

        dup2(to_c2[0],   STDIN_FILENO);
        dup2(from_c2[1], STDOUT_FILENO);
        close(to_c2[0]);
        close(from_c2[1]);

        char *const args[] = { (char *)FILTER_NAME, NULL };
        execv(filterpath, args);

        const char msg[] = "error: exec failed\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    close(to_c1[0]);
    close(to_c2[0]);
    close(from_c1[1]);
    close(from_c2[1]);

    char line[BUF_SIZE];
    char resp[BUF_SIZE];

    while (true) {
        ssize_t n = read_line(STDIN_FILENO, line, sizeof(line) - 1);
        if (n < 0) {
            const char msg[] = "error: read from stdin failed\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }
        if (n == 0) break;
        if (n == 1 && line[0] == '\n') break;

        bool to_first = roll_80();
        int wfd = to_first ? to_c1[1]   : to_c2[1];
        int rfd = to_first ? from_c1[0] : from_c2[0];

        if (write_all(wfd, line, n) == -1) {
            const char msg[] = "error: write to child failed\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            exit(EXIT_FAILURE);
        }

        ssize_t total = 0;
        while (total < n) {
            ssize_t r = read(rfd, resp + total, n - total);
            if (r < 0) {
                const char msg[] = "error: read from child failed\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                exit(EXIT_FAILURE);
            }
            if (r == 0) break;
            total += r;
        }

        write_all(STDOUT_FILENO, resp, total);
        if (out_fd != STDOUT_FILENO) write_all(out_fd, resp, total);
    }

    close(to_c1[1]);
    close(to_c2[1]);
    close(from_c1[0]);
    close(from_c2[0]);

    wait(NULL);
    wait(NULL);

    if (out_fd != STDOUT_FILENO) close(out_fd);

    return 0;
}
