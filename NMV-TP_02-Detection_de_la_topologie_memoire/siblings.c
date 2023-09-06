#include "hwdetect.h"

#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>


#define MEMORY_SIZE        (1ul << 20)
# define CACHELINE_SIZE 64
# define CACHE_SIZE 32768

// Just in case we add warmup phase
#define WARMUP         10000
#define PRECISION      1000000

static inline char *alloc(size_t n) {
    size_t i;
    char *ret = mmap(NULL, n, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ret == MAP_FAILED)
        abort();

    for (i = 0; i < n; i += PAGE_SIZE)
        ret[i] = 0;

    return ret;
}


double run(void *args) {
//    char *mem = ((char *) args);
    setcore(PARAM);

    // Trying to run some calculations in order to determine which core are paired together
    double x = 82.2;
    double result;
    result = sqrt(x);

//    for (int p = 0; p < PRECISION; p++)
//        for (int i = 0; i < (CACHE_SIZE / CACHELINE_SIZE); i += 1)
//            writemem(mem + i);

    return result;
}

static inline uint64_t detect(char *mem) {
//    size_t i, p;
    uint64_t start, end;
    pthread_t threads[1];


    start = now();
//    pthread_create(&threads[0], NULL, (void *(*)(void *)) run, mem);
    pthread_create(&threads[0], NULL, (void *) run, mem);
    pthread_join(threads[0], NULL);
    setcore(0);

//    for (p = 0; p < PRECISION; p++)
//        for (i = 0; i < (CACHE_SIZE / CACHELINE_SIZE); i += 1)
//            writemem(mem + i);
    int x = 81;
    double result;
    result = sqrt(x);

    end = now();

    return (end - start);
}


int main(void) {
    char *mem = alloc(MEMORY_SIZE);
    uint64_t t = detect(mem);

    printf("%d %lu\n", PARAM, t);
    return EXIT_SUCCESS;
}
