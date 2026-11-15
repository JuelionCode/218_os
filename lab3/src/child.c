#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>

static bool is_vowel(unsigned char c1, unsigned char c2, int *bytes_consumed) {
    *bytes_consumed = 1;

    char lower = (char)tolower(c1);
    if (lower == 'a' || lower == 'e' || lower == 'i' || 
        lower == 'o' || lower == 'u' || lower == 'y') {
        return true;
    }

    if (c1 == 0xD0 || c1 == 0xD1) {
        *bytes_consumed = 2;
        if (c1 == 0xD0 && c2 == 0x81) return true;
        if (c1 == 0xD1 && c2 == 0x91) return true;

        if (c1 == 0xD0) {
            if (c2 == 0x90 || c2 == 0x95 || c2 == 0x98 || c2 == 0x9E || 
                c2 == 0xA3 || c2 == 0xAB || c2 == 0xAD || c2 == 0xAE || c2 == 0xAF ||
                c2 == 0xB0 || c2 == 0xB5 || c2 == 0xB8 || c2 == 0xBE) {
                return true;
            }
        } else if (c1 == 0xD1) {
            if (c2 == 0x83 || c2 == 0x8B || c2 == 0x8D || c2 == 0x8E || c2 == 0x8F) {
                return true;
            }
        }
        return false;
    }

    return false;
}

static void filter_vowels(const char *src, size_t len, char *dst, size_t *out_len) {
    size_t r = 0;
    size_t w = 0;
    while (r < len) {
        unsigned char c1 = (unsigned char)src[r];
        unsigned char c2 = (r + 1 < len) ? (unsigned char)src[r + 1] : 0;
        int consumed = 1;

        if (is_vowel(c1, c2, &consumed)) {
            r += consumed;
        } else {
            for (int k = 0; k < consumed; ++k) {
                dst[w++] = src[r++];
            }
        }
    }
    dst[w] = '\0';
    *out_len = w;
}

int main(int argc, char *argv[]) {

    if (argc < 5) {
        const char msg[] = "Child error: invalid arguments\n";
        (void)!write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(EXIT_FAILURE);
    }

    const char *out_filename = argv[1];
    const char *shm_name = argv[2];
    const char *sem_empty_name = argv[3];
    const char *sem_full_name = argv[4];

    FILE *out_fp = fopen(out_filename, "w");
    if (!out_fp) {
        perror("Child: fopen output file failed");
        _exit(EXIT_FAILURE);
    }

    int shm_fd = shm_open(shm_name, O_RDWR, 0);
    if (shm_fd == -1) {
        perror("Child: shm_open failed");
        fclose(out_fp);
        _exit(EXIT_FAILURE);
    }

    ShmData *shm_data = mmap(NULL, sizeof(ShmData), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_data == MAP_FAILED) {
        perror("Child: mmap failed");
        close(shm_fd);
        fclose(out_fp);
        _exit(EXIT_FAILURE);
    }

    sem_t *sem_empty = sem_open(sem_empty_name, 0);
    if (sem_empty == SEM_FAILED) {
        perror("Child: sem_open empty failed");
        munmap(shm_data, sizeof(ShmData));
        close(shm_fd);
        fclose(out_fp);
        _exit(EXIT_FAILURE);
    }

    sem_t *sem_full = sem_open(sem_full_name, 0);
    if (sem_full == SEM_FAILED) {
        perror("Child: sem_open full failed");
        sem_close(sem_empty);
        munmap(shm_data, sizeof(ShmData));
        close(shm_fd);
        fclose(out_fp);
        _exit(EXIT_FAILURE);
    }

    char filtered[MAX_LINE_LEN];

    while (true) {
        if (sem_wait(sem_full) == -1) {
            perror("Child: sem_wait full failed");
            break;
        }

        if (shm_data->is_eof) {
            sem_post(sem_empty);
            break;
        }

        uint32_t len = shm_data->length;
        if (len > 0) {
            size_t out_len = 0;
            filter_vowels(shm_data->data, len, filtered, &out_len);

            if (out_len > 0) {
                fwrite(filtered, 1, out_len, out_fp);
                fflush(out_fp);
            }
        }

        sem_post(sem_empty);
    }

    fclose(out_fp);
    sem_close(sem_full);
    sem_close(sem_empty);
    munmap(shm_data, sizeof(ShmData));
    close(shm_fd);

    return 0;
}
