
#include <assert.h>
#include <atomic>
#include <chrono>
#include <pthread.h>
#include <random>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../src/deque.h"
#include "spec.h"
#include "stress.h"

#define THREAD_COUNT 50
#define FREEZE_COUNT 10000
#define N_TRIES      100

using namespace std;

/*
 * Basic idea:
 *  - allocate enough threads to exceed core count
 *  - each thread does rand ops to deque
 *  - every X ops, check consistency of deque
 *  - bomb out if errthang is fucked
 *  - after N tries, finish
 */

// globalz r evul :s
bounded_deque_t test_deque;
pthread_t threads[THREAD_COUNT];
thread_args_t args[THREAD_COUNT];
thread_stats_t stats[THREAD_COUNT];
pthread_barrier_t barrier;

void *rand_ops(void *args_void) {
    default_random_engine rand_engine(chrono::system_clock::now().time_since_epoch().count());
    uniform_real_distribution<double> value_dist(0.0, 65536.0);
    uniform_real_distribution<double> op_dist(0.0, 1.0);
    vector<int *> free_list;
    vector<int *>::iterator vi;
    
    // unpack args
    thread_args_t *thread_args = (thread_args_t *) args_void;
    int id = thread_args->id, i, j;
    
    // wait to launch thread work
    wait_on_barrier(&barrier);
    // do random thangs
    for(i = 0; i < N_TRIES; i++) {
        for(j = 0; j < FREEZE_COUNT; j++) {
            int op_status = OK;
            double random_op = op_dist(rand_engine);
            if(random_op < 0.5) {
                // push op
                int *random_value = (int *) malloc(sizeof(int));
                *random_value = (int) value_dist(rand_engine); 
                if(random_op < 0.25) {
                    left_push(test_deque, random_value, op_status);
                    if(op_status == OK) {
                        stats[id].left_pushes++;
                        free_list.push_back(random_value);
                    } else {
                        free(random_value);
                    }
                } else {
                    right_push(test_deque, random_value, op_status);
                    if(op_status == OK) {
                        stats[id].right_pushes++;
                        free_list.push_back(random_value);
                    } else {
                        free(random_value);
                    }
                }
            } else {
                // pop op
                int *popped_value;
                if(random_op < 0.75) {
                    popped_value = left_pop(test_deque, op_status);
                    if(op_status == OK)
                        stats[id].left_pops++;
                } else {
                    popped_value = right_pop(test_deque, op_status);
                    if(op_status == OK)
                        stats[id].right_pops++;
                }
            }
        }
        // allow main thread to scan
        wait_on_barrier(&barrier);
        wait_on_barrier(&barrier);
        // free old values
        for(vi = free_list.begin(); vi != free_list.end(); vi++)
            free(*vi);
        free_list.clear();
    }

    pthread_exit(NULL);
}

bool is_consistent(bounded_deque_t &deque, int &status_code) {
    int i, deque_section = LEFT_BLANK;
    
    // first scan: data struct invariants hold
    if(deque.nodes[0].load().value != LNULL) {
        status_code = LEFT_END_CORRUPT;
        return false;
    }
    if(deque.nodes[DEF_BOUNDS - 1].load().value != RNULL) {
        status_code = RIGHT_END_CORRUPT;
        return false;
    }
    for(i = 0; i < DEF_BOUNDS; i++) {
        bounded_deque_node_t current = deque.nodes[i].load();
        if(deque_section == LEFT_BLANK) {
            if(!is_null(current.value)) {
                // start of queue contents
                deque_section = NOT_BLANK;
            } else if(current.value == RNULL) {
                // empty expectation
                deque_section = RIGHT_BLANK;
            }
        } else if(deque_section == NOT_BLANK) {
            if(current.value == LNULL) {
                // inconsistent state!
                status_code = BROKEN_LEFT_SECTION;
                return false;
            } else if(current.value == RNULL) {
                // start of right section
                deque_section = RIGHT_BLANK;
            }
        } else if(deque_section == RIGHT_BLANK) {
            if(!is_null(current.value)) {
                // inconsistent state!
                status_code = BROKEN_RIGHT_SECTION;
                return false;
            } else if(current.value == LNULL) {
                // inconsistent state!
                status_code = MIXED_LEFT_RIGHT_SECTION;
                return false;
            }
        }
    }
    
    // second scan: # of elements gels with # of ops
    int actual_size = 0;
    unsigned long long int expected_size = 0;
    for(i = 0; i < THREAD_COUNT; i++) {
        /*fprintf(stdout, "\t\tThread %d ops:\n", i);
        fprintf(stdout, "\t\t\tleft_pushes: %lu\n", stats[i].left_pushes);
        fprintf(stdout, "\t\t\tleft_pops: %lu\n", stats[i].left_pops);
        fprintf(stdout, "\t\t\tright_pushes: %lu\n", stats[i].right_pushes);
        fprintf(stdout, "\t\t\tright_pops: %lu\n", stats[i].right_pops);*/
        expected_size += (stats[i].left_pushes + stats[i].right_pushes);
        expected_size -= (stats[i].left_pops + stats[i].right_pops);
    }
    for(i = 0; i < DEF_BOUNDS; i++) {
        bounded_deque_node_t current = deque.nodes[i].load();
        if(!is_null(current.value))
            actual_size++;
    }
    if(actual_size > (int) expected_size) {
        status_code = LOST_OPS;
        return false;
    }
    if(actual_size < (int) expected_size) {
        status_code = EXTRA_OPS;
        return false;
    }
    
    status_code = CONSISTENT;
    return true;
}

int main(int argc, char *argv[]) {
    int i, err_stat;
    
    fprintf(stdout, "Starting stress test...\n");
    fprintf(stdout, "\tInitializing barrier and threads\n");
    // init everything
    init_bounded_deque(test_deque);
    init_pthread_barrier(barrier, THREAD_COUNT + 1);
    for(i = 0; i < THREAD_COUNT; i++) {
        init_thread_args(args[i], i);
        init_thread_stats(stats[i]);
        start_pthread(threads[i], &rand_ops, args[i]);
    }
    fprintf(stdout, "\t\tInitial deque state:\n");
    fprintf(stdout, "\t\t\tsize: %lu\n", test_deque.size.load());
    fprintf(stdout, "\t\t\tleft_hint: %lu\n", test_deque.left_hint.load());
    fprintf(stdout, "\t\t\tright_hint: %lu\n", test_deque.right_hint.load());
    // launch thread work
    wait_on_barrier(&barrier);
    // wait on barrier, then scan
    for(i = 0; i < N_TRIES; i++) {
        fprintf(stdout, "\tTry & scan iteration %d\n", i + 1);
        wait_on_barrier(&barrier);
        if(!is_consistent(test_deque, err_stat)) {
            fprintf(stderr, "\t\tUh oh, inconsistent deque state...\n");
            switch(err_stat) {
                case LEFT_END_CORRUPT:
                    fprintf(stderr, "\t\t\tError: left end pointer corrupt\n");
                    break;
                case RIGHT_END_CORRUPT:
                    fprintf(stderr, "\t\t\tError: right end pointer corrupt\n");
                    break;
                case BROKEN_LEFT_SECTION:
                    fprintf(stderr, "\t\t\tError: found non-contiguous LNULLs\n");
                    break;
                case BROKEN_RIGHT_SECTION:
                    fprintf(stderr, "\t\t\tError: found non-contiguous RNULLs\n");
                    break;
                case MIXED_LEFT_RIGHT_SECTION:
                    fprintf(stderr, "\t\t\tError: found non-contiguous LNULLs/RNULLs\n");
                    break;
                case LOST_OPS:
                    fprintf(stderr, "\t\t\tError: # ops < size of deque\n");
                    break;
                case EXTRA_OPS:
                    fprintf(stderr, "\t\t\tError: # ops > size of deque\n");
                    break;
                default:
                    fprintf(stderr, "\t\t\twtf is going on?\n");
                    exit(0);
            }
            exit(err_stat);
        } else {
            fprintf(stdout, "\t\tDeque was consistent, current state:\n");
            fprintf(stdout, "\t\t\tsize: %lu\n", test_deque.size.load());
            fprintf(stdout, "\t\t\tleft_hint: %lu\n", test_deque.left_hint.load());
            fprintf(stdout, "\t\t\tright_hint: %lu\n", test_deque.right_hint.load());
        }
        wait_on_barrier(&barrier);
    }
    // test completed successfully!
    fprintf(stdout, "\tDeque consistent through all iterations! Exiting...\n");
    return 0;
}

