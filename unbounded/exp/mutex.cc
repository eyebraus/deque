
#include <assert.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <pthread.h>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <unistd.h>
#include <vector>
#include "../src/deque.h"
#include "experiment.h"
#include "mutex.h"
#include "atomic_ops.h"
#include "hrtimer/hrtimer_x86.h"

#define DEBUG 0

using namespace std;

deque<int> std_deque;
pthread_t *threads;
pthread_barrier_t barrier;
tas_lock_t lock;
int thread_count = 1;
int op_count = 10000;
int sec_count = 10;
experiment_type experiment = TIMING;
workflow_type workflow = RANDOM;
extern char *optarg;
bool stop = false;
atomic_int all_ops;
atomic_int push_ops;
atomic_int pop_ops;

/*
 * experiments:
 *     - timing
 *     - throughput
 * workflows:
 *     - stackish
 *     - queueish
 *     - random
 * thread count
 */

int main(int argc, char *argv[]) {
    int i, t, o;
    double result;
    string estr, wstr;
    
    while((i = getopt(argc, argv, "e:w:t:o:")) != -1) {
        switch(i) {
            // experiment type
            case 'e':
                estr = string(optarg);
                if(estr.compare("timing") == 0)
                    experiment = TIMING;
                else if(estr.compare("throughput") == 0)
                    experiment = THROUGHPUT;
                else
                    fprintf(stderr, "WARN: no such option -e %s, using default \"timing\"\n", optarg);
                break;
            // workflow type
            case 'w':
                wstr = string(optarg);
                if(wstr.compare("stack") == 0)
                    workflow = STACK;
                else if(wstr.compare("queue") == 0)
                    workflow = QUEUE;
                else if(wstr.compare("random") == 0)
                    workflow = RANDOM;
                else
                    fprintf(stderr, "WARN: no such option -w %s, using default \"random\"\n", optarg);
                break;
            // # threads
            case 't':
                t = atoi(optarg);
                if(t > 0)
                    thread_count = t;
                else
                    fprintf(stderr, "WARN: no such option -t %d, using default 1", t);
                break;
            // # ops or seconds
            case 'o':
                o = atoi(optarg);
                if(o > 0) {
                    if(experiment == TIMING)
                        op_count = o;
                    else if(experiment == THROUGHPUT)
                        sec_count = o;
                } else {
                    if(experiment == TIMING)
                        fprintf(stderr, "WARN: no such option -o %d, using default %d", o, op_count);
                    else if(experiment == THROUGHPUT)
                        fprintf(stderr, "WARN: no such option -o %d, using default %d", o, sec_count);
                }
                break;
        }
    }
    
    // initialize important stuff
    std_deque = deque<int>();
    int boost = experiment == TIMING ? 0 : 1;
    op_count *= thread_count;
    threads = (pthread_t *) malloc(sizeof(pthread_t) * (thread_count + boost));
    pthread_barrier_init(&barrier, NULL, thread_count + boost); 
    result = 0.0;
    lock = 0;
    all_ops = 0;
    push_ops = 0;
    pop_ops = 0;
    
    // perform experiments
    switch(experiment) {
        // TODO: get results from ops
        case TIMING:
            result = timing_exp();
            break;
        case THROUGHPUT:
            result = throughput_exp();
            break;
        default:
            fprintf(stderr, "ERR: bad experiment mode detected.\n");
            exit(-1);
            break;
    }
    
    // output result
    output_results(result);
    
    return 0;
}

double timing_exp() {
    int i, stat;
    void *retval;
    double result;
    thread_args_t *targs;
    
    // spawn worker threads
    targs = (thread_args_t *) malloc(sizeof(thread_args_t) * thread_count);
    for(i = 0; i < thread_count; i++) {
        targs[i].id = i;
        if(stat = pthread_create(&threads[i], NULL, &timing_run, (void *) &targs[i])) {
            fprintf(stderr, "ERR: I couldn't create thread %d for you :( [STAT: %d]\n", i, stat);
            exit(-1);
        }
    }
    
    // wait on results from 0th thread
    pthread_join(threads[0], &retval);
    result = *(double *) retval;
    free(targs);
    free(retval);
    
    return result;
}

double throughput_exp() {
    int i, stat;
    void *retval;
    double result = 0.0;
    thread_args_t *targs;
    
    // spawn worker threads
    targs = (thread_args_t *) malloc(sizeof(thread_args_t) * (thread_count + 1));
    for(i = 0; i < thread_count; i++) {
        targs[i].id = i;
        if(stat = pthread_create(&threads[i], NULL, &throughput_run, (void *) &targs[i])) {
            fprintf(stderr, "ERR: I couldn't create thread %d for you :( [STAT: %d]\n", i, stat);
            exit(-1);
        }
    }
    
    // spawn timing thread
    targs[thread_count].id = thread_count;
    if(stat = pthread_create(&threads[thread_count], NULL, &throughput_kill, (void *) &targs[thread_count])) {
        fprintf(stderr, "ERR: I couldn't create thread %d for you :( [STAT: %d]\n", thread_count, stat);
        exit(-1);
    }
    
    // wait on results from all threads
    for(i = 0; i < thread_count; i++) {
        pthread_join(threads[i], &retval);
        result += *(double *) retval;
        free(retval);
    }
    free(targs);
    
    // take # ops per milli, then avg of that
    return result / (double) thread_count;
}

void *timing_run(void *args_void) {
    thread_args_t *args;
    int id, i, push_ptr, push_status, pop_status;
    double start, finish;
    atomic_int ops;
    
    args = (thread_args_t *) args_void;
    id = args->id;
    ops = 0;
    push_ptr = 0xbeef;
    
    // random number stuff
    default_random_engine rand_engine(chrono::system_clock::now().time_since_epoch().count());
    uniform_real_distribution<double> op_dist(0.0, 1.0);
    
    // wait for all threads to arrive
    pthread_barrier_wait(&barrier);
    if(id == 0)
        start = gethrtime_x86();
    
    // actually work now
    if(workflow == STACK) {
        // phase 1: pushes
        if(id < thread_count / 2) {
            for(; push_ops < op_count / 2;) {
                tas_acquire(&lock);
                std_deque.push_back(push_ptr);
                tas_release(&lock);
                push_ops++;
            }
        }
        //pthread_barrier_wait(&barrier);
        // phase 2: pops
        if(id >= thread_count / 2) {
            for(; pop_ops < op_count / 2;) {
                tas_acquire(&lock);
                if(!std_deque.empty()) {
                    std_deque.pop_back();
                    pop_ops++;
                }
                tas_release(&lock);
            } 
        }
    } else if(workflow == QUEUE) {
        // phase 1: pushes
        if(id < thread_count / 2) {
            for(; push_ops < op_count / 2;) {
                tas_acquire(&lock);
                std_deque.push_front(push_ptr);
                tas_release(&lock);
                push_ops++;
            }
        }
        //pthread_barrier_wait(&barrier);
        // phase 2: pops
        if(id >= thread_count / 2) {
            for(; pop_ops < op_count / 2;) {
                tas_acquire(&lock);
                if(!std_deque.empty()) {
                    std_deque.pop_back();
                    pop_ops++;
                }
                tas_release(&lock);
                ops++;
            } 
        }
    } else {
        for(; all_ops < op_count; ) {
            double random_op = op_dist(rand_engine);
            if(random_op < 0.25) {
                tas_acquire(&lock);
                std_deque.push_back(push_ptr);
                tas_release(&lock);
                all_ops++;
            } else if(random_op < 0.5) {
                tas_acquire(&lock);
                std_deque.push_front(push_ptr);
                tas_release(&lock);
                all_ops++;
            } else if(random_op < 0.75) {
                tas_acquire(&lock);
                if(!std_deque.empty()) {
                    std_deque.pop_back();
                    all_ops++;
                }
                tas_release(&lock);
            } else {
                tas_acquire(&lock);
                if(!std_deque.empty()) {
                    std_deque.pop_front();
                    all_ops++;
                }
                tas_release(&lock);
            }
        }
    }
    
    // wait for all threads to finish
    pthread_barrier_wait(&barrier);
    if(id == 0) {
        finish = gethrtime_x86();
        double *result = (double *) malloc(sizeof(double));
        *result = (finish - start) * 1000.0;
        pthread_exit((void *) result);
    } else {
        pthread_exit(NULL);
    }
}

void *throughput_run(void *args_void) {
    thread_args_t *args;
    int id, i, push_ptr, push_status, pop_status;
    atomic_int ops;
    
    args = (thread_args_t *) args_void;
    id = args->id;
    ops = 0;
    push_ptr = 0xbeef;
    
    // random number stuff
    default_random_engine rand_engine(chrono::system_clock::now().time_since_epoch().count());
    uniform_real_distribution<double> op_dist(0.0, 1.0);
    
    // wait for all threads to arrive
    pthread_barrier_wait(&barrier);
    
    // actually work now
    if(workflow == STACK) {
        // phase 1: pushes
        if(id < thread_count / 2) {
            while(!stop) {
                tas_acquire(&lock);
                std_deque.push_back(push_ptr);
                tas_release(&lock);
                ops++;
            }
        }
        //pthread_barrier_wait(&barrier);
        // phase 2: pops
        if(id >= thread_count / 2) {
            while(!stop) {
                tas_acquire(&lock);
                if(!std_deque.empty()) {
                    std_deque.pop_back();
                    ops++;
                }
                tas_release(&lock);
                ops++;
            } 
        }
    } else if(workflow == QUEUE) {
        // phase 1: pushes
        if(id < thread_count / 2) {
            while(!stop) {
                tas_acquire(&lock);
                std_deque.push_back(push_ptr);
                tas_release(&lock);
                ops++;
            }
        }
        //pthread_barrier_wait(&barrier);
        // phase 2: pops
        if(id >= thread_count / 2) {
            while(!stop) {
                tas_acquire(&lock);
                if(!std_deque.empty()) {
                    std_deque.pop_front();
                    ops++;
                }
                tas_release(&lock);
                ops++;
            } 
        }
    } else {
        while(!stop) {
            double random_op = op_dist(rand_engine);
            if(random_op < 0.25) {
                tas_acquire(&lock);
                std_deque.push_back(push_ptr);
                tas_release(&lock);
                ops++;
            } else if(random_op < 0.5) {
                tas_acquire(&lock);
                std_deque.push_front(push_ptr);
                tas_release(&lock);
                ops++;
            } else if(random_op < 0.75) {
                tas_acquire(&lock);
                if(!std_deque.empty()) {
                    std_deque.pop_back();
                    ops++;
                }
                tas_release(&lock);
                ops++;
            } else {
                tas_acquire(&lock);
                if(!std_deque.empty()) {
                    std_deque.pop_front();
                    ops++;
                }
                tas_release(&lock);
                ops++;
            }
        }
    }
    
    double *result = (double *) malloc(sizeof(double));
    *result = (double) ops / ((double) sec_count * 1000.0);
    pthread_exit((void *) result);
}

void *throughput_kill(void *args_void) {
    struct timespec sleep_time;
    sleep_time.tv_sec = sec_count;
    sleep_time.tv_nsec = 0;
    pthread_barrier_wait(&barrier);
    nanosleep(&sleep_time, NULL);
    stop = true;
    pthread_exit(NULL);
}

void output_results(double result) {
    if(experiment == TIMING) {
        fprintf(stdout, "Execution time: %.5f milliseconds\n", result);
    } else if(experiment == THROUGHPUT) {
        fprintf(stdout, "Average throughput: %.5f ops per millisecond\n", result);
    } else {
        fprintf(stdout, "wtf is goin' on\n");
        fprintf(stderr, "ERR: unknown experiment mode\n");
    }
}

