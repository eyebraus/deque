
#include <pthread.h>

typedef struct thread_args_struct {
    unsigned int id;
    pthread_barrier_t *barrier;
} thread_args_t;

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

