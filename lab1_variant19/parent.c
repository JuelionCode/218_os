#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>

static int read_line(char *buf, int max_len) {
    int len = 0;
    char ch;
    while (read(0, &ch, 1) > 0) {
        if (ch == '\n') {
            break;
        }
        if (len < max_len - 1) {
            buf[len++] = ch;
        }
    }
    buf[len] = '\0';
    return len;
}

int main(void) {
    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        const char msg[] = "error: failed to create pipes\n";
        write(2, msg, sizeof(msg) - 1);
        exit(1);
    }

    const char msg1[] = "Enter filename for child1: ";
    write(1, msg1, sizeof(msg1) - 1);
    char filename1[256];
    if (read_line(filename1, sizeof(filename1)) == 0) {
        const char msg[] = "error: empty filename 1\n";
        write(2, msg, sizeof(msg) - 1);
        exit(1);
    }

    const char msg2[] = "Enter filename for child2: ";
    write(1, msg2, sizeof(msg2) - 1);
    char filename2[256];
    if (read_line(filename2, sizeof(filename2)) == 0) {
        const char msg[] = "error: empty filename 2\n";
        write(2, msg, sizeof(msg) - 1);
        exit(1);
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        const char msg[] = "error: failed to fork child 1\n";
        write(2, msg, sizeof(msg) - 1);
        exit(1);
    } else if (pid1 == 0) {
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);

        dup2(pipe1[0], 0);
        close(pipe1[0]);

        char *args[] = {"child", filename1, NULL};
        execv("./child", args);

        const char msg[] = "error: failed to execute child 1\n";
        write(2, msg, sizeof(msg) - 1);
        _exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        const char msg[] = "error: failed to fork child 2\n";
        write(2, msg, sizeof(msg) - 1);
        exit(1);
    } else if (pid2 == 0) {
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[1]);

        dup2(pipe2[0], 0);
        close(pipe2[0]);

        char *args[] = {"child", filename2, NULL};
        execv("./child", args);

        const char msg[] = "error: failed to execute child 2\n";
        write(2, msg, sizeof(msg) - 1);
        _exit(1);
    }

    close(pipe1[0]);
    close(pipe2[0]);

    srand((unsigned int)time(NULL));

    char line[4096];
    int len = 0;
    char ch;

    while (read(0, &ch, 1) > 0) {
        line[len++] = ch;
        if (ch == '\n' || len == sizeof(line) - 1) {
            int prob = rand() % 100;
            if (prob < 80) {
                write(pipe1[1], line, len);
            } else {
                write(pipe2[1], line, len);
            }
            len = 0;
        }
    }

    if (len > 0) {
        int prob = rand() % 100;
        if (prob < 80) {
            write(pipe1[1], line, len);
        } else {
            write(pipe2[1], line, len);
        }
    }

    close(pipe1[1]);
    close(pipe2[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}
