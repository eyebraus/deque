
#include <pthread.h>

static inline void wait_on_pthread(pthread_t &thread, void **ret) {
    int stat;
    if(stat = pthread_join(thread, ret)) {
        fprintf(stderr, "Uh oh, I couldn't join thread for you :( [ERRNO: %d]\n", stat);
        exit(-1);
    }
}

static inline void init_pthread_barrier(pthread_barrier_t &barrier, int thr) {
    int stat;
    if(stat = pthread_barrier_init(&barrier, NULL, thr)) {
        fprintf(stderr, "Uh oh, I couldn't create a barrier for you :( [ERRNO: %d]\n", stat);
        exit(-1);
    }
}

static inline void wait_on_barrier(pthread_barrier_t *barrier) {
    int barrier_stat;
    barrier_stat = pthread_barrier_wait(barrier);
    if(barrier_stat != 0 && barrier_stat != PTHREAD_BARRIER_SERIAL_THREAD) {
        fprintf(stderr, "Uh oh, couldn't wait on barrier for some reason :( [ERRNO: %d]\n", barrier_stat);
        exit(-1);
    }
}
