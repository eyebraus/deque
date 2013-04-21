
#include <assert.h>
#include <atomic>
#include <pthread.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../src/deque.h"
#include "spec.h"

#define SPEC_COUNT 7

using namespace std;

bounded_deque_t test_deque;

typedef struct spec_result_struct {
    int size;
    int *results;
} spec_result_t;

/*
 * Specs:
 *     01. two threads push left 7 times in total 
 *     02. two threads push right 7 times in total 
 *         ensure:
 *             - all inserted elements are in deque
 *             - no unknown elements are in deque
 *             - deque is of expected size
 *             - MAYBE: ensure the intra-thread partial ordering???
 *     03. two threads pop left from full queue 7 times in total 
 *     04. two threads pop right from full queue 7 times in total
 *         ensure:
 *             - all expected elements were found
 *             - no unexpected elements were found
 *             - deque is of expected size
 *             - MAYBE: ensure the intra-thread partial ordering???
 *     05. two threads pop from full queue from each end until empty
 *         ensure:
 *             - elements are not popped twice when ends collide
 *             - ends do not "cross over"
 *     06. one thread pushes left 7 times, one thread pops right 7 times
 *     07. one thread pushes right 7 times, one thread pops left 7 times
 *         ensure:
 *             - set of elements popped = set of elements pushed
 *             - set of elements popped arrives in proper order
 *             - ends do not "cross over"
 *     ...
 */

void *spec001_helper(void *args_void) {
    unsigned int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    int i, push_status;

    assert(id == 1 || id == 2);
    
    // sync starting times of threads
    wait_on_barrier(barrier);
    
    if(id == 1) {
        for(i = 0; i < 4; i++) {
            fprintf(stdout, "\tPush iteration %3d\n", i + 1);
            left_push(test_deque, i + 1, push_status);
            assert(push_status == OK);
        }
    } else {
        for(i = 4; i < 7; i++) {
            fprintf(stdout, "\tPush iteration %3d\n", i + 1);
            left_push(test_deque, i + 1, push_status);
            assert(push_status == OK);
        }
    }
    
    pthread_exit(NULL);
}

int spec001() {
    pthread_t thread1, thread2;
    thread_args_t thread_args1, thread_args2;
    pthread_barrier_t barrier;
    int expected_init[] = { 1, 2, 3, 4, 5, 6, 7 }, i, start, finish;
    set<int> expected(expected_init, expected_init + 7);
    
    fprintf(stdout, "Spec %3d\n", 1);
    init_pthread_barrier(barrier, 2);
    init_thread_args(thread_args1, 1, &barrier);
    init_thread_args(thread_args2, 2, &barrier);
    start_pthread(thread1, &spec001_helper, thread_args1);
    start_pthread(thread2, &spec001_helper, thread_args2);
    wait_on_pthread(thread1, NULL);
    wait_on_pthread(thread2, NULL);
    
    assert(test_deque.size == expected.size());
    assert(test_deque.left_hint == DEF_BOUNDS / 2 - 8);
    assert(test_deque.right_hint == DEF_BOUNDS / 2);
    start = (int) test_deque.left_hint.load() + 1;
    finish = (int) test_deque.right_hint.load();
    for(i = start; i < finish; i++) {
        // check for node value set membership
        fprintf(stdout, "\t\tValidity assertion %3d\n", i);
        assert(expected.find(*test_deque.nodes[i].load().value) != expected.end());
    }
    
    return 0;
}

void *spec002_helper(void *args_void) {
    unsigned int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    int i, push_status;

    assert(id == 1 || id == 2);
    
    // sync starting times of threads
    wait_on_barrier(barrier);
    
    if(id == 1) {
        for(i = 0; i < 4; i++) {
            fprintf(stdout, "\tPush iteration %3d\n", i + 1);
            right_push(test_deque, i + 1, push_status);
            assert(push_status == OK);
        }
    } else {
        for(i = 4; i < 7; i++) {
            fprintf(stdout, "\tPush iteration %3d\n", i + 1);
            right_push(test_deque, i + 1, push_status);
            assert(push_status == OK);
        }
    }
    
    pthread_exit(NULL);
}

int spec002() {
    pthread_t thread1, thread2;
    thread_args_t thread_args1, thread_args2;
    pthread_barrier_t barrier;
    int expected_init[] = { 1, 2, 3, 4, 5, 6, 7 }, i, start, finish;
    set<int> expected(expected_init, expected_init + 7);
    
    fprintf(stdout, "Spec %3d\n", 2);
    init_pthread_barrier(barrier, 2);
    init_thread_args(thread_args1, 1, &barrier);
    init_thread_args(thread_args2, 2, &barrier);
    start_pthread(thread1, &spec002_helper, thread_args1);
    start_pthread(thread2, &spec002_helper, thread_args2);
    wait_on_pthread(thread1, NULL);
    wait_on_pthread(thread2, NULL);
    
    assert(test_deque.size == expected.size());
    assert(test_deque.left_hint == DEF_BOUNDS / 2 - 1);
    assert(test_deque.right_hint == DEF_BOUNDS / 2 + 7);
    start = (int) test_deque.left_hint.load() + 1;
    finish = (int) test_deque.right_hint.load();
    for(i = start; i < finish; i++) {
        // check for node value set membership
        assert(expected.find(*test_deque.nodes[i].load().value) != expected.end());
    }
    
    return 0;
}

void *spec003_helper(void *args_void) {
    unsigned int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    int i, pop_status, result, *results;

    assert(id == 1 || id == 2);
    results = (int *) malloc(sizeof(int) * (id == 1 ? 4 : 3));
    
    // sync starting times of threads
    wait_on_barrier(barrier);
    
    if(id == 1) {
        for(i = 0; i < 4; i++) {
            fprintf(stdout, "\tPop iteration %3d\n", i + 1);
            result = left_pop(test_deque, pop_status);
            results[i] = result;
            assert(pop_status == OK);
        }
    } else {
        for(i = 4; i < 7; i++) {
            fprintf(stdout, "\tPop iteration %3d\n", i + 1);
            result = left_pop(test_deque, pop_status);
            results[i - 4] = result;
            assert(pop_status == OK);
        }
    }
    
    pthread_exit((void *) results);
}

int spec003() {
    pthread_t thread1, thread2;
    thread_args_t thread_args1, thread_args2;
    pthread_barrier_t barrier;
    int expected_init[] = { 1, 2, 3, 4, 5, 6, 7 }, p, push_status;
    void *rvoid1, *rvoid2;
    set<int> expected(expected_init, expected_init + 7);
    set<int>::iterator i;
    
    fprintf(stdout, "Spec %3d\n", 3);
    for(p = 0; p < 7; p++) {
        fprintf(stdout, "\tPush iteration %3d\n", p + 1);
        left_push(test_deque, expected_init[p], push_status);
        assert(push_status == OK);
    }
    init_pthread_barrier(barrier, 2);
    init_thread_args(thread_args1, 1, &barrier);
    init_thread_args(thread_args2, 2, &barrier);
    start_pthread(thread1, &spec003_helper, thread_args1);
    start_pthread(thread2, &spec003_helper, thread_args2);
    wait_on_pthread(thread1, &rvoid1);
    wait_on_pthread(thread2, &rvoid2);
    set<int> results1((int *) rvoid1, (int *) rvoid1 + 4);
    set<int> results2((int *) rvoid2, (int *) rvoid2 + 3);
    
    assert(test_deque.size == 0);
    assert(test_deque.left_hint == DEF_BOUNDS / 2 - 1);
    assert(test_deque.right_hint == DEF_BOUNDS / 2);
    // results1 | results2 = expected
    for(i = results1.begin(); i != results1.end(); i++)
        assert(expected.find(*i) != expected.end());
    for(i = results2.begin(); i != results2.end(); i++)
        assert(expected.find(*i) != expected.end());
    // results1 & results2 = NULL
    for(i = results1.begin(); i != results1.end(); i++)
        assert(results2.find(*i) == results2.end());
    
    free(rvoid1);
    free(rvoid2);
    return 0;
}

void *spec004_helper(void *args_void) {
    unsigned int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    int i, pop_status, result, *results;

    assert(id == 1 || id == 2);
    results = (int *) malloc(sizeof(int) * (id == 1 ? 4 : 3));
    
    // sync starting times of threads
    wait_on_barrier(barrier);
    
    if(id == 1) {
        for(i = 0; i < 4; i++) {
            fprintf(stdout, "\tPop iteration %3d\n", i + 1);
            result = right_pop(test_deque, pop_status);
            results[i] = result;
            assert(pop_status == OK);
        }
    } else {
        for(i = 4; i < 7; i++) {
            fprintf(stdout, "\tPop iteration %3d\n", i + 1);
            result = right_pop(test_deque, pop_status);
            results[i - 4] = result;
            assert(pop_status == OK);
        }
    }
    
    pthread_exit((void *) results);
}

int spec004() {
    pthread_t thread1, thread2;
    thread_args_t thread_args1, thread_args2;
    pthread_barrier_t barrier;
    int expected_init[] = { 1, 2, 3, 4, 5, 6, 7 }, p, push_status;
    void *rvoid1, *rvoid2;
    set<int> expected(expected_init, expected_init + 7);
    set<int>::iterator i;
    
    fprintf(stdout, "Spec %3d\n", 4);
    for(p = 0; p < 7; p++) {
        fprintf(stdout, "\tPush iteration %3d\n", p + 1);
        right_push(test_deque, expected_init[p], push_status);
        assert(push_status == OK);
    }
    init_pthread_barrier(barrier, 2);
    init_thread_args(thread_args1, 1, &barrier);
    init_thread_args(thread_args2, 2, &barrier);
    start_pthread(thread1, &spec004_helper, thread_args1);
    start_pthread(thread2, &spec004_helper, thread_args2);
    wait_on_pthread(thread1, &rvoid1);
    wait_on_pthread(thread2, &rvoid2);
    set<int> results1((int *) rvoid1, (int *) rvoid1 + 4);
    set<int> results2((int *) rvoid2, (int *) rvoid2 + 3);
    
    assert(test_deque.size == 0);
    assert(test_deque.left_hint == DEF_BOUNDS / 2 - 1);
    assert(test_deque.right_hint == DEF_BOUNDS / 2);
    // results1 | results2 = expected
    for(i = results1.begin(); i != results1.end(); i++)
        assert(expected.find(*i) != expected.end());
    for(i = results2.begin(); i != results2.end(); i++)
        assert(expected.find(*i) != expected.end());
    // results1 & results2 = NULL
    for(i = results1.begin(); i != results1.end(); i++)
        assert(results2.find(*i) == results2.end());
    
    free(rvoid1);
    free(rvoid2);
    return 0;
}

void *spec005_helper(void *args_void) {
    unsigned int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    int i = 0, pop_status, result, *results;
    spec_result_t *spec_result = (spec_result_t *) malloc(sizeof(spec_result_t));

    assert(id == 1 || id == 2);
    results = (int *) malloc(sizeof(int) * (DEF_BOUNDS - 2));
    
    // sync starting times of threads
    wait_on_barrier(barrier);
    
    if(id == 1) {
        while(true) {
            fprintf(stdout, "\tLeft pop iteration %3d\n", i + 1);
            result = left_pop(test_deque, pop_status);
            if(pop_status == EMPTY)
                break;
            results[i++] = result;
            assert(pop_status == OK);
        }
    } else {
        while(true) {
            fprintf(stdout, "\tRight pop iteration %3d\n", i + 1);
            result = right_pop(test_deque, pop_status);
            if(pop_status == EMPTY)
                break;
            results[i++] = result;
            assert(pop_status == OK);
        }
    }
    
    spec_result->results = results;
    spec_result->size = i;
    pthread_exit((void *) spec_result);
}

int spec005() {
    pthread_t thread1, thread2;
    thread_args_t thread_args1, thread_args2;
    pthread_barrier_t barrier;
    int p, push_status;
    void *rvoid1, *rvoid2;
    spec_result_t *results1, *results2;
    set<int> expected;
    set<int>::iterator i;
    fprintf(stdout, "Spec %3d\n", 5);
    
    for(p = 0; p < 15; p++) {
        fprintf(stdout, "\tPush iteration %3d\n", p + 1);
        left_push(test_deque, (p + 1) * -1, push_status);
        assert(push_status == OK);
        right_push(test_deque, p + 1, push_status);
        assert(push_status == OK);
        expected.insert((p + 1) * -1);
        expected.insert(p + 1);
        assert(push_status == OK);
    }
    init_pthread_barrier(barrier, 2);
    init_thread_args(thread_args1, 1, &barrier);
    init_thread_args(thread_args2, 2, &barrier);
    start_pthread(thread1, &spec005_helper, thread_args1);
    start_pthread(thread2, &spec005_helper, thread_args2);
    wait_on_pthread(thread1, &rvoid1);
    wait_on_pthread(thread2, &rvoid2);
    results1 = (spec_result_t *) rvoid1;
    results2 = (spec_result_t *) rvoid2;
    set<int> results1_set(results1->results, results1->results + results1->size);
    set<int> results2_set(results2->results, results2->results + results2->size);
    
    assert(test_deque.size == 0);
    assert(test_deque.left_hint < test_deque.right_hint);
    // results1 | results2 = expected
    for(i = results1_set.begin(); i != results1_set.end(); i++)
        assert(expected.find(*i) != expected.end());
    for(i = results2_set.begin(); i != results2_set.end(); i++)
        assert(expected.find(*i) != expected.end());
    for(i = expected.begin(); i != expected.end(); i++)
        assert(results1_set.find(*i) != results1_set.end() || results2_set.find(*i) != results2_set.end());
    // results1 & results2 = NULL
    for(i = results1_set.begin(); i != results1_set.end(); i++)
        assert(results2_set.find(*i) == results2_set.end());
    
    free(results1->results);
    free(results1);
    free(results2->results);
    free(results2);
    return 0;
}

void *spec006_helper(void *args_void) {
    unsigned int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    int i, push_status, pop_status, result, *results = (int *) malloc(sizeof(int) * 7);
    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 25;

    assert(id == 1 || id == 2);
    
    // sync starting times of threads
    wait_on_barrier(barrier);
    
    if(id == 1) {
        for(i = 0; i < 7; i++) {
            fprintf(stdout, "\tPush iteration %3d\n", i + 1);
            left_push(test_deque, i + 1, push_status);
            results[i] = i + 1;
            assert(push_status == OK);
        }
    } else {
        for(i = 0; i < 7; ) {
            fprintf(stdout, "\tPop iteration %3d\n", i + 1);
            result = right_pop(test_deque, pop_status);
            if(pop_status == EMPTY) {
                fprintf(stdout, "\t\tQueue was empty; continuning...\n");
                nanosleep(&sleep_time, NULL);
            } else {
                assert(pop_status == OK);
                results[i++] = result;
            }
        }
    }
    
    pthread_exit(results);
}

int spec006() {
    pthread_t thread1, thread2;
    thread_args_t thread_args1, thread_args2;
    pthread_barrier_t barrier;
    void *rvoid1, *rvoid2;
    int *results1, *results2, i;
    
    fprintf(stdout, "Spec %3d\n", 6);
    init_pthread_barrier(barrier, 2);
    init_thread_args(thread_args1, 1, &barrier);
    init_thread_args(thread_args2, 2, &barrier);
    start_pthread(thread1, &spec006_helper, thread_args1);
    start_pthread(thread2, &spec006_helper, thread_args2);
    wait_on_pthread(thread1, &rvoid1);
    wait_on_pthread(thread2, &rvoid2);
    results1 = (int *) rvoid1;
    results2 = (int *) rvoid2;
    
    assert(test_deque.size == 0);
    assert(test_deque.left_hint < test_deque.right_hint);
    assert(test_deque.left_hint == test_deque.right_hint - 1);
    // ensure same order
    for(i = 0; i < 7; i++) {
        assert(results1[i] == results2[i]);
    }
    
    free(results1);
    free(results2);
    return 0;
}

void *spec007_helper(void *args_void) {
    unsigned int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    int i, push_status, pop_status, result, *results = (int *) malloc(sizeof(int) * 7);
    struct timespec sleep_time;
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = 25;

    assert(id == 1 || id == 2);
    
    // sync starting times of threads
    wait_on_barrier(barrier);
    
    if(id == 1) {
        for(i = 0; i < 7; i++) {
            fprintf(stdout, "\tPush iteration %3d\n", i + 1);
            right_push(test_deque, i + 1, push_status);
            results[i] = i + 1;
            assert(push_status == OK);
        }
    } else {
        for(i = 0; i < 7; ) {
            fprintf(stdout, "\tPop iteration %3d\n", i + 1);
            result = left_pop(test_deque, pop_status);
            if(pop_status == EMPTY) {
                fprintf(stdout, "\t\tQueue was empty; continuning...\n");
                nanosleep(&sleep_time, NULL);
            } else {
                assert(pop_status == OK);
                results[i++] = result;
            }
        }
    }
    
    pthread_exit(results);
}

int spec007() {
    pthread_t thread1, thread2;
    thread_args_t thread_args1, thread_args2;
    pthread_barrier_t barrier;
    void *rvoid1, *rvoid2;
    int *results1, *results2, i;
    
    fprintf(stdout, "Spec %3d\n", 7);
    init_pthread_barrier(barrier, 2);
    init_thread_args(thread_args1, 1, &barrier);
    init_thread_args(thread_args2, 2, &barrier);
    start_pthread(thread1, &spec007_helper, thread_args1);
    start_pthread(thread2, &spec007_helper, thread_args2);
    wait_on_pthread(thread1, &rvoid1);
    wait_on_pthread(thread2, &rvoid2);
    results1 = (int *) rvoid1;
    results2 = (int *) rvoid2;
    
    assert(test_deque.size == 0);
    assert(test_deque.left_hint < test_deque.right_hint);
    assert(test_deque.left_hint == test_deque.right_hint - 1);
    // ensure same order
    for(i = 0; i < 7; i++) {
        assert(results1[i] == results2[i]);
    }
    
    free(results1);
    free(results2);
    return 0;
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
