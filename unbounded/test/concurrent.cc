
#include <assert.h>
#include <atomic>
#include <pthread.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../src/deque.h"
#include "concurrent.h"
#include "spec.h"

#define THREAD_COUNT 16
#define SPEC_COUNT 7

using namespace std;

deque_t test_deque;

/*
 * Specs:
 *     01. two thread pools push left 64 times in total
 *     02. two thread pools push right 64 times in total
 *         ensure:
 *             - all inserted elements are in deque
 *             - no unknown elements are in deque
 *             - actual heads are reachable from hints
 *             - left is reachable from right, and vice versa
 *     03. two thread pools pop left 64 times in total
 *     04. two thread pools pop right 64 times in total
 *         ensure:
 *             - all expected elements are found
 *             - no unexpected elements are found
 *             - actual heads are reachable from hints
 *             - left is reachable from right, and vice versa
 *     05. two thread pools pop from each end until empty
 *         ensure:
 *             - elements are not popped twice
 *             - ends do not cross over
 *     06. one thread pool pushes left 64 times, one thread pool keeps popping right
 *     07. one thread pool pushes right 64 times, one thread pool keeps popping left
 *         ensure:
 *             - elements popped = elements pushed
 *             - order of elts popped = order of elts pushed
 *             - ends do not cross over
 */

//     01. two thread pools push left 64 times in total
int spec001() {
    int i;
    thread_pool_t pool1, pool2;
    thread_args_t targs1, targs2;
    pthread_barrier_t barrier;
    spec_results_t *results1, *results2;
    set<int> expected;
    deque_hint_t left_hint, right_hint;
    atomic_deque_node_t *left_buffer, *right_buffer;
    long int left_head, right_head;
    bool left_check, head_found, unreachable;
    
    fprintf(stdout, "Spec %3d\n", 1);
    for(i = 1; i <= 64; i++)
        expected.insert(i);
    init_pthread_barrier(barrier, THREAD_COUNT * 2);
    init_thread_pool(pool1, 0, THREAD_COUNT);
    init_thread_pool(pool2, 1, THREAD_COUNT);
    init_thread_args(targs1, 1, &barrier);
    init_thread_args(targs2, 2, &barrier);
    start_thread_pool(pool1, &spec001_helper, targs1);
    start_thread_pool(pool2, &spec001_helper, targs2);
    results1 = wait_on_thread_pool(pool1);
    results2 = wait_on_thread_pool(pool2);
    
    //right_hint = test_deque.right_hint.load();
    // actual heads are reachable from hints
    left_hint = test_deque.left_hint.load();
    left_buffer = left_hint.nodes;
    left_head = left_hint.index;
    left_check = true;
    head_found = false;
    unreachable = false;
    while(!head_found && !unreachable) {
        if(left_check) {
            // check over the current buffer
            for(i = left_head; mod(i, DEF_BOUNDS) >= 0; i--) {
                deque_node_t previous, current;
                current = left_buffer[mod(i, DEF_BOUNDS)].load();
                previous = left_buffer[mod(i + 1, DEF_BOUNDS)].load();
                if(current.value == LNULL && previous.value != LNULL) {
                    head_found = true;
                    break;
                }
            }
            // move to next buffer if possible
            left_buffer = (atomic_deque_node_t *) left_buffer[0].load().value;
            left_head = 
        } else {

        }
    }
    // left is reachable from right, right from left

    // all inserted elements are in deque
    // + no unknown elements are in deque

}

int spec002() {

}

int spec003() {

}

int spec004() {

}

int spec005() {

}

int spec006() {

}

int spec007() {

}

int main(int argc, char *argv[]) {
    int (*specs[SPEC_COUNT])(void) = { &spec001, &spec002, &spec003, &spec004, &spec005, &spec006, &spec007 };
    int i, spec_stat;
    
    for(i = 0; i < SPEC_COUNT; i++) {
        init_bounded_deque(test_deque);
        spec_stat = specs[i]();
        clear_bounded_deque(test_deque);
    }
    
    return 0;
}
