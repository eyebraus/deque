
#include <pthread.h>
//#include "../test/spec.h"

typedef struct thread_args_struct {
    unsigned int id;
    pthread_barrier_t *barrier;
} thread_args_t;

typedef struct thread_pool_struct {
    int id;
    int size;
    pthread_t *threads;
    thread_args_t *args;
} thread_pool_t;

typedef struct spec_result_struct {
    int size;
    int **results;
} spec_result_t;

static inline void init_thread_pool(thread_pool_t &pool, int id, int size) {
    pool.id = id;
    pool.size = size;
    pool.threads = (pthread_t *) malloc(sizeof(pthread_t) * size);
    pool.args = (thread_args_t *) malloc(sizeof(thread_args_t) * size);
}

static inline void init_thread_args(thread_args_t &args, unsigned int id, pthread_barrier_t *barrier) {
    args.id = id;
    args.barrier = barrier;
}

static inline void start_pthread(pthread_t &thread, void *(*proc)(void *), thread_args_t &args) {
    int stat;
    if(stat = pthread_create(&thread, NULL, proc, (void *) &args)) {
        fprintf(stderr, "Uh oh, I couldn't create thread for you :( [ERRNO: %d]\n", stat);
        exit(-1);
    }
}

static inline void start_thread_pool(thread_pool_t &pool, void *(*proc)(void *), thread_args_t &args) {
    int i;
    for(i = 0; i < pool.size; i++) {
        pool.args[i].id = (pool.id - 1) * pool.size + i;
        pool.args[i].barrier = args.barrier;
        start_pthread(pool.threads[i], proc, pool.args[i]);
    }
}

static inline spec_result_t *wait_on_thread_pool(thread_pool_t &pool) {
    int i;
    void *rvoid;
    spec_result_t *results = (spec_result_t *) malloc(sizeof(spec_result_t) * pool.size);
    spec_result_t *result;
    for(i = 0; i < pool.size; i++) {
        wait_on_pthread(pool.threads[i], &rvoid);
        result = (spec_result_t *) rvoid;
        if(result != NULL) {
            results[i].size = result->size;
            results[i].results = result->results;
            free(result);
        } else {
            results[i].size = 0;
            results[i].results = NULL;
        }
    }
    return results;
}

