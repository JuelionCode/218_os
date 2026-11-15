#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define SHM_SIZE 4096

#define SHM_PREFIX_1 "/lab3_shm1_"
#define SHM_PREFIX_2 "/lab3_shm2_"
#define SEM_EMPTY_PREFIX_1 "/lab3_sem_empty1_"
#define SEM_FULL_PREFIX_1  "/lab3_sem_full1_"
#define SEM_EMPTY_PREFIX_2 "/lab3_sem_empty2_"
#define SEM_FULL_PREFIX_2  "/lab3_sem_full2_"

#define MAX_LINE_LEN (SHM_SIZE - sizeof(uint32_t) - sizeof(uint32_t))

typedef struct {
    uint32_t length;
    uint32_t is_eof;
    char data[MAX_LINE_LEN];
} ShmData;

#endif
