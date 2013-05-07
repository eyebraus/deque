
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

#define DEBUG 0
#define THREAD_COUNT 50
#define FREEZE_COUNT 1
#define N_TRIES      10000

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
deque_t test_deque;
pthread_t threads[THREAD_COUNT];
thread_args_t args[THREAD_COUNT];
thread_stats_t stats[THREAD_COUNT];
pthread_barrier_t barrier;

void dump_queue(deque_t &deque) {
    int i, deque_section = LEFT_BLANK, start;
    deque_hint_t left_hint, right_hint;
    atomic_deque_node_t *buffer, *left_buffer, *right_buffer;
    long int left_head, right_head;
    vector<atomic_deque_node_t *> left_buffer_chain, right_buffer_chain;
    vector<atomic_deque_node_t *>::iterator left_iter;
    vector<atomic_deque_node_t *>::reverse_iterator right_iter;
    
    left_hint = test_deque.left_hint.load();
    left_buffer = left_hint.nodes;
    left_head = left_hint.index;
    right_hint = test_deque.right_hint.load();
    right_buffer = right_hint.nodes;
    right_head = right_hint.index;
    
    buffer = left_buffer;
    while(buffer[0].load().value != LNULL)
        buffer = (atomic_deque_node_t *) buffer[0].load().value;
    while(buffer != (atomic_deque_node_t *) RNULL) {
        left_buffer_chain.push_back(buffer);
        buffer = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
    }
    
    buffer = right_buffer;
    while(buffer[DEF_BOUNDS - 1].load().value != RNULL)
        buffer = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
    while(buffer != (atomic_deque_node_t *) LNULL) {
        right_buffer_chain.push_back(buffer);
        buffer = (atomic_deque_node_t *) buffer[0].load().value;
    }
    
    for(left_iter = left_buffer_chain.begin(); left_iter != left_buffer_chain.end(); left_iter++) {
        atomic_deque_node_t *nodes = *left_iter;
        fprintf(stdout, "\t\tBuffer %p\n", nodes);
        for(i = 0; i < DEF_BOUNDS; i++) {
            deque_node_t current = nodes[i].load();
            if(i == 0 || i == DEF_BOUNDS - 1)
                fprintf(stdout, "\t\t\tB %d: %p\n", i, current.value);
            else if(left_iter == left_buffer_chain.begin() && i < mod(left_head, DEF_BOUNDS))
                fprintf(stdout, "\t\t\tX %d: %p\n", i, current.value);
            else if(left_iter == left_buffer_chain.end() && i > mod(right_head, DEF_BOUNDS))
                fprintf(stdout, "\t\t\tX %d: %p\n", i, current.value);
            else
                fprintf(stdout, "\t\t\t  %d: %p\n", i, current.value);
        }
    }
}

void *rand_ops(void *args_void) {
    default_random_engine rand_engine(chrono::system_clock::now().time_since_epoch().count());
    uniform_real_distribution<double> value_dist(0.0, 65536.0);
    uniform_real_distribution<double> op_dist(0.0, 1.0);
    
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
                    if(DEBUG && THREAD_COUNT <= 4) fprintf(stdout, "\t\tLEFT PUSH\n");
                    left_push(test_deque, random_value, op_status);
                    if(op_status == OK) {
                        stats[id].left_pushes++;
                    } else {
                        free(random_value);
                    }
                } else {
                    if(DEBUG && THREAD_COUNT <= 4) fprintf(stdout, "\t\tRIGHT PUSH\n");
                    right_push(test_deque, random_value, op_status);
                    if(op_status == OK) {
                        stats[id].right_pushes++;
                    } else {
                        free(random_value);
                    }
                }
            } else {
                // pop op
                int *popped_value;
                if(random_op < 0.75) {
                    if(DEBUG && THREAD_COUNT <= 4) fprintf(stdout, "\t\tLEFT POP\n");
                    popped_value = left_pop(test_deque, op_status);
                    if(op_status == OK) {
                        stats[id].left_pops++;
                        free(popped_value);
                    } else {
                        //assert(popped_value == NULL);
                    }
                } else {
                    if(DEBUG && THREAD_COUNT <= 4) fprintf(stdout, "\t\tRIGHT POP\n");
                    popped_value = right_pop(test_deque, op_status);
                    if(op_status == OK) {
                        stats[id].right_pops++;
                        free(popped_value);
                    } else {
                        //assert(popped_value == NULL);
                    }
                }
            }
        }
        // allow main thread to scan
        wait_on_barrier(&barrier);
        wait_on_barrier(&barrier);
    }

    pthread_exit(NULL);
}

bool is_consistent(deque_t &deque, int &status_code) {
    int i, deque_section = LEFT_BLANK;
    deque_hint_t left_hint, right_hint;
    atomic_deque_node_t *buffer, *left_buffer, *right_buffer;
    long int left_head, right_head;
    vector<atomic_deque_node_t *> left_buffer_chain, right_buffer_chain;
    vector<atomic_deque_node_t *>::iterator left_iter;
    vector<atomic_deque_node_t *>::reverse_iterator right_iter;
    
    // first scan: left hint & right hint are in same chain
    left_hint = test_deque.left_hint.load();
    left_buffer = left_hint.nodes;
    left_head = left_hint.index;
    right_hint = test_deque.right_hint.load();
    right_buffer = right_hint.nodes;
    right_head = right_hint.index;
    
    buffer = left_buffer;
    while(buffer[0].load().value != LNULL)
        buffer = (atomic_deque_node_t *) buffer[0].load().value;
    while(buffer != (atomic_deque_node_t *) RNULL) {
        left_buffer_chain.push_back(buffer);
        buffer = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
    }
    
    buffer = right_buffer;
    while(buffer[DEF_BOUNDS - 1].load().value != RNULL)
        buffer = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
    while(buffer != (atomic_deque_node_t *) LNULL) {
        right_buffer_chain.push_back(buffer);
        buffer = (atomic_deque_node_t *) buffer[0].load().value;
    }
    
    left_iter = left_buffer_chain.begin();
    right_iter = right_buffer_chain.rbegin();
    while(left_iter != left_buffer_chain.end() && right_iter != right_buffer_chain.rend()) {
        //fprintf(stdout, "\tLeft chain: %p, right chain: %p\n", *left_iter, *right_iter);
        if(*left_iter != *right_iter) {
            status_code = UNREACHABLE;
            return false;
        }
        left_iter++;
        right_iter++;
    }

    // second scan: deque invariants hold (LNULL+ .* RNULL+)
    for(left_iter = left_buffer_chain.begin(); left_iter != left_buffer_chain.end(); left_iter++) {
        atomic_deque_node_t *nodes = *left_iter;
        for(i = 0; i < DEF_BOUNDS; i++) {
            deque_node_t current = nodes[i].load();
            if(i == 0) {
                if(current.value == RNULL) {
                    // inconsistent state!
                    status_code = LEFT_END_CORRUPT;
                    return false;
                }
                if(left_iter != left_buffer_chain.begin() && current.value != (void *) *(left_iter - 1)) {
                    // inconsistent state!
                    status_code = LEFT_END_CORRUPT;
                    return false;
                }
            } else if(i == DEF_BOUNDS - 1) {
                if(current.value == LNULL) {
                    // inconsistent state!
                    status_code = RIGHT_END_CORRUPT;
                    return false;
                }
                if(left_iter + 1 != left_buffer_chain.end() && current.value != (void *) *(left_iter + 1)) {
                    // inconsistent state!
                    status_code = RIGHT_END_CORRUPT;
                    return false;
                }
            } else if(deque_section == LEFT_BLANK) {
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
    }
    
    // third scan: # of elements gels with # of ops
    long long int actual_size = 0;
    long long int expected_size = 0;
    for(i = 0; i < THREAD_COUNT; i++) {
        /*fprintf(stdout, "\t\tThread %d ops:\n", i);
        fprintf(stdout, "\t\t\tleft_pushes: %lu\n", stats[i].left_pushes);
        fprintf(stdout, "\t\t\tleft_pops: %lu\n", stats[i].left_pops);
        fprintf(stdout, "\t\t\tright_pushes: %lu\n", stats[i].right_pushes);
        fprintf(stdout, "\t\t\tright_pops: %lu\n", stats[i].right_pops);*/
        expected_size += (stats[i].left_pushes + stats[i].right_pushes);
        expected_size -= (stats[i].left_pops + stats[i].right_pops);
    }
    for(left_iter = left_buffer_chain.begin(); left_iter != left_buffer_chain.end(); left_iter++) {
        atomic_deque_node_t *nodes = *left_iter;
        for(i = 1; i < DEF_BOUNDS - 1; i++) {
            deque_node_t current = nodes[i].load();
            if(!is_null(current.value))
                actual_size++;
        }
    }
    if(actual_size > expected_size) {
        fprintf(stdout, "\t\tActual: %lld, Expected: %lld\n", actual_size, expected_size);
        status_code = LOST_OPS;
        return false;
    }
    if(actual_size < expected_size) {
        fprintf(stdout, "\t\tActual: %lld, Expected: %lld\n", actual_size, expected_size);
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
    init_deque(test_deque);
    init_pthread_barrier(barrier, THREAD_COUNT + 1);
    for(i = 0; i < THREAD_COUNT; i++) {
        init_thread_args(args[i], i);
        init_thread_stats(stats[i]);
        start_pthread(threads[i], &rand_ops, args[i]);
    }
    fprintf(stdout, "\t\tInitial deque state:\n");
    fprintf(stdout, "\t\t\tsize: %lu\n", test_deque.size.load());
    fprintf(stdout, "\t\t\tleft_hint: { nodes: %p, index: %ld }\n", test_deque.left_hint.load().nodes, test_deque.left_hint.load().index);
    fprintf(stdout, "\t\t\tright_hint: { nodes: %p, index: %ld }\n", test_deque.right_hint.load().nodes, test_deque.right_hint.load().index);
    // launch thread work
    wait_on_barrier(&barrier);
    // wait on barrier, then scan
    for(i = 0; i < N_TRIES; i++) {
        fprintf(stdout, "\tTry & scan iteration %d\n", i + 1);
        wait_on_barrier(&barrier);
        if(!is_consistent(test_deque, err_stat)) {
            fprintf(stderr, "\t\tUh oh, inconsistent deque state...\n");
            fprintf(stdout, "\t\t\tsize: %lu\n", test_deque.size.load());
            fprintf(stdout, "\t\t\tleft_hint: { nodes: %p, index: %ld }\n", test_deque.left_hint.load().nodes, test_deque.left_hint.load().index);
            fprintf(stdout, "\t\t\tright_hint: { nodes: %p, index: %ld }\n", test_deque.right_hint.load().nodes, test_deque.right_hint.load().index);
            switch(err_stat) {
                case LEFT_END_CORRUPT:
                    fprintf(stderr, "\t\t\tError: left end pointer corrupt\n");
                    break;
                case RIGHT_END_CORRUPT:
                    fprintf(stderr, "\t\t\tError: right end pointer corrupt\n");
                    break;
                case BROKEN_LEFT_SECTION:
                    fprintf(stderr, "\t\t\tError: found non-contiguous LNULLs\n");
                    dump_queue(test_deque);
                    break;
                case BROKEN_RIGHT_SECTION:
                    fprintf(stderr, "\t\t\tError: found non-contiguous RNULLs\n");
                    dump_queue(test_deque);
                    break;
                case MIXED_LEFT_RIGHT_SECTION:
                    fprintf(stderr, "\t\t\tError: found non-contiguous LNULLs/RNULLs\n");
                    break;
                case LOST_OPS:
                    fprintf(stderr, "\t\t\tError: # ops < size of deque\n");
                    dump_queue(test_deque);
                    break;
                case EXTRA_OPS:
                    fprintf(stderr, "\t\t\tError: # ops > size of deque\n");
                    dump_queue(test_deque);
                    break;
                case UNREACHABLE:
                    fprintf(stderr, "\t\t\tError: left end could not be reached from right end\n");
                    break;
                default:
                    fprintf(stderr, "\t\t\twtf is going on?\n");
                    exit(0);
            }
            exit(err_stat);
        } else {
            fprintf(stdout, "\t\tDeque was consistent, current state:\n");
            fprintf(stdout, "\t\t\tsize: %lu\n", test_deque.size.load());
            fprintf(stdout, "\t\t\tleft_hint: { nodes: %p, index: %ld }\n", test_deque.left_hint.load().nodes, test_deque.left_hint.load().index);
            fprintf(stdout, "\t\t\tright_hint: { nodes: %p, index: %ld }\n", test_deque.right_hint.load().nodes, test_deque.right_hint.load().index);
        }
        wait_on_barrier(&barrier);
    }
    // test completed successfully!
    fprintf(stdout, "\tDeque consistent through all iterations! Exiting...\n");
    return 0;
}

