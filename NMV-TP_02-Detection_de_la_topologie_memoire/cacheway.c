#include "hwdetect.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>


#define MEMORY_SIZE        (1ul << 20)
# define CACHELINE_SIZE 32
# define CACHE_SIZE 32768

// Just in case we add warmup phase
#define WARMUP         10000
#define PRECISION      1000000


static inline char *alloc(void) {
    size_t i;
    char *ret = mmap(NULL, MEMORY_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ret == MAP_FAILED)
        abort();

    for (i = 0; i < MEMORY_SIZE; i += PAGE_SIZE)
        ret[i] = 0;

    return ret;
}

static inline uint64_t detect(char *mem) {
    size_t i, j, p;
    uint64_t start, end;

    for (p = 0; p < WARMUP; p++)
        for (i = 0; i < (CACHE_SIZE / CACHELINE_SIZE) / PARAM; i += 1)
            for (j = 0; j < PARAM; j += 1)
                writemem(mem + (j * CACHE_SIZE) + (i*CACHELINE_SIZE));

    start = now();

    for (p = 0; p < PRECISION; p++)
        for (i = 0; i < (CACHE_SIZE / CACHELINE_SIZE) / PARAM; i += 1)
            for (j = 0; j < PARAM; j += 1)
                writemem(mem + (j * CACHE_SIZE) + (i*CACHELINE_SIZE));

    end = now();

    return (end - start);
}

int main(void) {
    char *mem = alloc();
    uint64_t t = detect(mem);

    printf("%d %lu\n", PARAM, t);
    return EXIT_SUCCESS;
}
