
#include <pthread.h>

typedef struct thread_stats_struct {
    unsigned long long int left_pushes;
    unsigned long long int right_pushes;
    unsigned long long int left_pops;
    unsigned long long int right_pops;
} thread_stats_t;

typedef struct thread_args_struct {
    unsigned int id;
} thread_args_t;

typedef enum { LEFT_BLANK, NOT_BLANK, RIGHT_BLANK } scan_state;
typedef enum { CONSISTENT, LEFT_END_CORRUPT, RIGHT_END_CORRUPT,
    BROKEN_LEFT_SECTION, BROKEN_RIGHT_SECTION, MIXED_LEFT_RIGHT_SECTION,
    LOST_OPS, EXTRA_OPS, UNREACHABLE } scan_errors;

static inline void init_thread_args(thread_args_t &args, unsigned int id) {
    args.id = id;
}

static inline void init_thread_stats(thread_stats_t &stats) {
    stats.left_pushes = 0;
    stats.right_pushes = 0;
    stats.left_pops = 0;
    stats.right_pops = 0;
}

static inline void start_pthread(pthread_t &thread, void *(*proc)(void *), thread_args_t &args) {
    int stat;
    if(stat = pthread_create(&thread, NULL, proc, (void *) &args)) {
        fprintf(stderr, "Uh oh, I couldn't create thread for you :( [ERRNO: %d]\n", stat);
        exit(-1);
    }
}

