
#include <pthread.h>

typedef struct thread_args_struct {
    unsigned int id;
    pthread_barrier_t *barrier;
} thread_args_t;

typedef struct thread_pool_struct {
    int id;
    int size;
    pthread_t *threads;
} thread_pool_t;

static inline void init_thread_pool(thread_pool_t &pool, int id, int size, thread_args_t *args) {
    pool.id = id;
    pool.size = size;
    pool.threads = (pthread_t *) malloc(sizeof(pthread_t) * size);
}

static inline void init_thread_args(thread_args_t &args, unsigned int id, pthread_barrier_t *barrier) {
    args.id = id;
    args.barrier = barrier;
}

static inline void start_thread_pool(thread_pool_t &pool, void *(*proc)(void *), thread_args_t &args) {
    
}

static inline void start_pthread(pthread_t &thread, void *(*proc)(void *), thread_args_t &args) {
    int stat;
    if(stat = pthread_create(&thread, NULL, proc, (void *) &args)) {
        fprintf(stderr, "Uh oh, I couldn't create thread for you :( [ERRNO: %d]\n", stat);
        exit(-1);
    }
}

