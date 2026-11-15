#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>

typedef struct {
    char shm_name[128];
    char sem_empty_name[128];
    char sem_full_name[128];
    int shm_fd;
    ShmData *shm_ptr;
    sem_t *sem_empty;
    sem_t *sem_full;
    pid_t pid;
} ChildChannel;

static void init_channel(ChildChannel *ch, const char *shm_pref, const char *sem_emp_pref, const char *sem_full_pref, pid_t parent_pid) {

    snprintf(ch->shm_name, sizeof(ch->shm_name), "%s%d", shm_pref, (int)parent_pid);
    snprintf(ch->sem_empty_name, sizeof(ch->sem_empty_name), "%s%d", sem_emp_pref, (int)parent_pid);
    snprintf(ch->sem_full_name, sizeof(ch->sem_full_name), "%s%d", sem_full_pref, (int)parent_pid);

    shm_unlink(ch->shm_name);
    sem_unlink(ch->sem_empty_name);
    sem_unlink(ch->sem_full_name);


    ch->shm_fd = shm_open(ch->shm_name, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (ch->shm_fd == -1) {
        perror("Parent: shm_open create failed");
        exit(EXIT_FAILURE);
    }


    if (ftruncate(ch->shm_fd, sizeof(ShmData)) == -1) {
        perror("Parent: ftruncate failed");
        close(ch->shm_fd);
        shm_unlink(ch->shm_name);
        exit(EXIT_FAILURE);
    }

    ch->shm_ptr = (ShmData *)mmap(NULL, sizeof(ShmData), PROT_READ | PROT_WRITE, MAP_SHARED, ch->shm_fd, 0);
    if (ch->shm_ptr == MAP_FAILED) {
        perror("Parent: mmap failed");
        close(ch->shm_fd);
        shm_unlink(ch->shm_name);
        exit(EXIT_FAILURE);
    }

    memset(ch->shm_ptr, 0, sizeof(ShmData));


    ch->sem_empty = sem_open(ch->sem_empty_name, O_CREAT | O_EXCL, 0666, 1);
    if (ch->sem_empty == SEM_FAILED) {
        perror("Parent: sem_open empty failed");
        munmap(ch->shm_ptr, sizeof(ShmData));
        close(ch->shm_fd);
        shm_unlink(ch->shm_name);
        exit(EXIT_FAILURE);
    }

    ch->sem_full = sem_open(ch->sem_full_name, O_CREAT | O_EXCL, 0666, 0);
    if (ch->sem_full == SEM_FAILED) {
        perror("Parent: sem_open full failed");
        sem_close(ch->sem_empty);
        sem_unlink(ch->sem_empty_name);
        munmap(ch->shm_ptr, sizeof(ShmData));
        close(ch->shm_fd);
        shm_unlink(ch->shm_name);
        exit(EXIT_FAILURE);
    }
}

static void send_to_child(ChildChannel *ch, const char *line, size_t len, bool is_eof) {
    sem_wait(ch->sem_empty);

    ch->shm_ptr->length = (uint32_t)len;
    ch->shm_ptr->is_eof = is_eof ? 1 : 0;
    if (len > 0) {
        memcpy(ch->shm_ptr->data, line, len);
    }

    sem_post(ch->sem_full);
}

static void cleanup_channel(ChildChannel *ch) {
    if (ch->sem_full != SEM_FAILED) {
        sem_close(ch->sem_full);
        sem_unlink(ch->sem_full_name);
    }
    if (ch->sem_empty != SEM_FAILED) {
        sem_close(ch->sem_empty);
        sem_unlink(ch->sem_empty_name);
    }
    if (ch->shm_ptr != MAP_FAILED) {
        munmap(ch->shm_ptr, sizeof(ShmData));
    }
    if (ch->shm_fd != -1) {
        close(ch->shm_fd);
        shm_unlink(ch->shm_name);
    }
}

int main(void) {

    srand((unsigned int)time(NULL) ^ (unsigned int)getpid());

    char file1[256];
    char file2[256];

    if (!fgets(file1, sizeof(file1), stdin)) {
        return 0;
    }
    file1[strcspn(file1, "\r\n")] = '\0';

    if (!fgets(file2, sizeof(file2), stdin)) {
        return 0;
    }
    file2[strcspn(file2, "\r\n")] = '\0';

    pid_t parent_pid = getpid();

    ChildChannel ch1;
    ChildChannel ch2;

    init_channel(&ch1, SHM_PREFIX_1, SEM_EMPTY_PREFIX_1, SEM_FULL_PREFIX_1, parent_pid);
    init_channel(&ch2, SHM_PREFIX_2, SEM_EMPTY_PREFIX_2, SEM_FULL_PREFIX_2, parent_pid);

    ch1.pid = fork();
    if (ch1.pid == -1) {
        perror("Parent: fork 1 failed");
        cleanup_channel(&ch1);
        cleanup_channel(&ch2);
        exit(EXIT_FAILURE);
    }
    if (ch1.pid == 0) {
        char *args[] = {
            "./child",
            file1,
            ch1.shm_name,
            ch1.sem_empty_name,
            ch1.sem_full_name,
            NULL
        };
        execv("./child", args);
        perror("Parent: execv child 1 failed");
        _exit(EXIT_FAILURE);
    }

    ch2.pid = fork();
    if (ch2.pid == -1) {
        perror("Parent: fork 2 failed");
        send_to_child(&ch1, NULL, 0, true);
        waitpid(ch1.pid, NULL, 0);
        cleanup_channel(&ch1);
        cleanup_channel(&ch2);
        exit(EXIT_FAILURE);
    }
    if (ch2.pid == 0) {
        char *args[] = {
            "./child",
            file2,
            ch2.shm_name,
            ch2.sem_empty_name,
            ch2.sem_full_name,
            NULL
        };
        execv("./child", args);
        perror("Parent: execv child 2 failed");
        _exit(EXIT_FAILURE);
    }

    char line_buf[MAX_LINE_LEN];
    while (fgets(line_buf, sizeof(line_buf), stdin)) {
        size_t len = strlen(line_buf);
        if (len == 0) continue;

        int r = rand() % 100;
        if (r < 80) {
            send_to_child(&ch1, line_buf, len, false);
        } else {
            send_to_child(&ch2, line_buf, len, false);
        }
    }

    send_to_child(&ch1, NULL, 0, true);
    send_to_child(&ch2, NULL, 0, true);

    waitpid(ch1.pid, NULL, 0);
    waitpid(ch2.pid, NULL, 0);

    cleanup_channel(&ch1);
    cleanup_channel(&ch2);

    return 0;
}
