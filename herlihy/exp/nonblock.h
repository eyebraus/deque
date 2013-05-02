
typedef struct thread_args_struct {
    pthread_barrier_t *barrier;
} thread_args_t;

typedef struct thread_results_struct {
    unsigned long long pushes;
    unsigned long long pops;
    double push_time;
    double pop_time;
} thread_results_t;
