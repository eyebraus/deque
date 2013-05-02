
#include <assert.h>
#include <atomic>
#include <pthread.h>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include "../src/deque.h"
#include "nonblock.h"
#include "hrtimer/hrtimer_x86.h"

#define DEBUG 1
#define THREAD_COUNT 64
#define OP_TYPE 1

using namespace std;

bounded_deque_t test_deque;
bool stop;

void *sleeper(void *args_void) {
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    struct timespec sleep_time;
    sleep_time.tv_sec = 10;
    sleep_time.tv_nsec = 0;
    pthread_barrier_wait(barrier);
    nanosleep(&sleep_time, NULL);
    stop = true;
    pthread_exit(NULL);
}

void *run(void *args_void) {
    bool left_shove = true, right_shove = true;
    int *push_ptr = (int *) malloc(sizeof(int));
    *push_ptr = 0xbeef;
    int push_status = OK, pop_status = OK;
    unsigned long long pushes = 0, pops = 0;
    double start, finish;
    double push_time, pop_time;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    thread_results_t *results = (thread_results_t *) malloc(sizeof(thread_results_t));
    
    pthread_barrier_wait(barrier);
    while(!stop) {
        if(left_shove) {
            start = gethrtime_x86();
            left_push(test_deque, push_ptr, push_status);
            finish = gethrtime_x86();
            push_time += (finish - start);
            if(push_status == FULL) {
                left_shove = false;
            }
            pushes++;
        } else {
            start = gethrtime_x86();
            int *popped_ptr = left_pop(test_deque, pop_status);
            finish = gethrtime_x86();
            pop_time += (finish - start);
            if(pop_status == EMPTY) {
                left_shove = true;
            }
            pops++;
        }

        if(right_shove) {
            start = gethrtime_x86();
            right_push(test_deque, push_ptr, push_status);
            finish = gethrtime_x86();
            push_time += (finish - start);
            if(push_status == FULL) {
                right_shove = false;
            }
            pushes++;
        } else {
            start = gethrtime_x86();
            int *popped_ptr = right_pop(test_deque, pop_status);
            finish = gethrtime_x86();
            pop_time += (finish - start);
            if(pop_status == EMPTY) {
                right_shove = true;
            }
            pops++;
        }
        
    }
    
    results->pushes = pushes;
    results->pops = pops;
    results->push_time = push_time * 1000.0;
    results->pop_time = pop_time * 1000.0;
    pthread_exit((void *) results);
}

int main(int argc, char *argv[]) {
    int i;
    pthread_t master;
    thread_args_t args_master;
    pthread_t threads[THREAD_COUNT];
    thread_args_t args[THREAD_COUNT];
    thread_results_t results[THREAD_COUNT];
    pthread_barrier_t barrier;
    
    stop = false;
    init_bounded_deque(test_deque); 
    pthread_barrier_init(&barrier, NULL, THREAD_COUNT + 1);
    for(i = 0; i < THREAD_COUNT; i++) {
        args[i].barrier = &barrier;
        pthread_create(&threads[i], NULL, &run, (void *) &args);
    }
    args_master.barrier = &barrier;
    pthread_create(&master, NULL, &sleeper, (void *) &args);
    for(i = 0; i < THREAD_COUNT; i++) {
        void *retval;
        pthread_join(threads[i], &retval);
        thread_results_t *result = (thread_results_t *) retval;
        results[i].pushes = result->pushes; 
        results[i].pops = result->pops; 
        results[i].push_time = result->push_time; 
        results[i].pop_time = result->pop_time; 
    }
    for(i = 0; i < THREAD_COUNT; i++) {
        fprintf(stdout, "\t%d %llu %.5f %llu %.5f\n", i, results[i].pushes, results[i].push_time, results[i].pops, results[i].pop_time);
    }
}
