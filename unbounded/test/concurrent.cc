
#include <assert.h>
#include <atomic>
#include <pthread.h>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include "../src/deque.h"
#include "spec.h"
#include "concurrent.h"

#define DEBUG 1
#define THREAD_COUNT 32
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

void *spec001_helper(void *args_void) {
    int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    //fprintf(stdout, "\t\tThread %d barrier %p\n", id, barrier);
    int i, push_status;

    wait_on_barrier(barrier);
    int *push_value = (int *) malloc(sizeof(int));
    *push_value = id + 1;
    left_push(test_deque, push_value, push_status);
    assert(push_status == OK);
    wait_on_barrier(barrier);
    
    pthread_exit(NULL);
}

//     01. two thread pools push left 64 times in total
int spec001() {
    int i;
    thread_pool_t pool1, pool2;
    thread_args_t targs1, targs2;
    pthread_barrier_t barrier;
    spec_result_t *results1, *results2;
    void **rvoid1, **rvoid2;
    set<int> expected, found;
    set<int>::iterator set_iter;
    vector<atomic_deque_node_t *> left_buffer_chain, right_buffer_chain;
    vector<atomic_deque_node_t *>::iterator left_iter;
    vector<atomic_deque_node_t *>::reverse_iterator right_iter;
    deque_hint_t left_hint, right_hint;
    atomic_deque_node_t *left_buffer, *right_buffer, *last_buffer;
    long int left_head, right_head, last_head;
    bool left_check, right_check, head_found, unreachable;
    
    fprintf(stdout, "Spec %3d\n", 1);
    for(i = 1; i <= THREAD_COUNT * 2; i++)
        expected.insert(i);
    init_pthread_barrier(barrier, THREAD_COUNT * 2);
    init_thread_pool(pool1, 1, THREAD_COUNT);
    init_thread_pool(pool2, 2, THREAD_COUNT);
    init_thread_args(targs1, 1, &barrier);
    init_thread_args(targs2, 2, &barrier);
    start_thread_pool(pool1, &spec001_helper, targs1);
    start_thread_pool(pool2, &spec001_helper, targs2);
    rvoid1 = wait_on_thread_pool(pool1);
    rvoid2 = wait_on_thread_pool(pool2);
    
    // both hints see exactly the same buffer chain
    left_hint = test_deque.left_hint.load();
    left_buffer = left_hint.nodes;
    left_head = left_hint.index;
    right_hint = test_deque.right_hint.load();
    right_buffer = right_hint.nodes;
    right_head = right_hint.index;
    atomic_deque_node_t *iter = left_buffer;
    while(iter[0].load().value != LNULL)
        iter = (atomic_deque_node_t *) iter[0].load().value;
    while(iter != (atomic_deque_node_t *) RNULL) {
        left_buffer_chain.push_back(iter);
        iter = (atomic_deque_node_t *) iter[DEF_BOUNDS - 1].load().value;
    }
    iter = right_buffer;
    while(iter[DEF_BOUNDS - 1].load().value != RNULL)
        iter = (atomic_deque_node_t *) iter[DEF_BOUNDS - 1].load().value;
    while(iter != (atomic_deque_node_t *) LNULL) {
        right_buffer_chain.push_back(iter);
        iter = (atomic_deque_node_t *) iter[0].load().value;
    }
    left_iter = left_buffer_chain.begin();
    right_iter = right_buffer_chain.rbegin();
    while(left_iter != left_buffer_chain.end() && right_iter != right_buffer_chain.rend()) {
        if(DEBUG) fprintf(stdout, "\tLeft chain: %p, right chain: %p\n", *left_iter, *right_iter);
        assert(*left_iter == *right_iter);
        left_iter++;
        right_iter++;
    }
    
    // actual heads are reachable from hints
    left_check = true;
    head_found = false;
    unreachable = false;
    while(!head_found) {
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
            last_buffer = left_buffer;
            last_head = left_head;
            left_buffer = (atomic_deque_node_t *) left_buffer[0].load().value;
            left_head = i - 1;
            // switch to right check if next ptr is null
            if(left_buffer == LNULL)
                left_check = false;
        } else {
            // check over the current buffer
            for(i = left_head; mod(i, DEF_BOUNDS) < DEF_BOUNDS - 1; i++) {
                deque_node_t next, current;
                current = left_buffer[mod(i, DEF_BOUNDS)].load();
                next = left_buffer[mod(i + 1, DEF_BOUNDS)].load();
                if(current.value == LNULL && next.value != LNULL) {
                    head_found = (next.value == RNULL);
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = left_buffer;
            last_head = left_head;
            left_buffer = (atomic_deque_node_t *) left_buffer[DEF_BOUNDS - 1].load().value;
            left_head = i + 2;
        }
    }
    assert(head_found);
    long int cindex = mod(last_head, DEF_BOUNDS), pindex = mod(last_head + 1, DEF_BOUNDS);
    fprintf(stdout, "\t\tLeft head: %p @ %ld\n", last_buffer, last_head);
    assert(last_buffer[cindex].load().value == LNULL);
    assert(last_buffer[pindex].load().value != LNULL);
    fprintf(stdout, "\t\t\tCurrent: %p, Previous: %p\n", last_buffer[cindex].load().value, last_buffer[pindex].load().value);
    
    right_check = true;
    head_found = false;
    unreachable = false;
    while(!head_found) {
        if(right_check) {
            // check over the current buffer
            for(i = right_head; mod(i, DEF_BOUNDS) <= DEF_BOUNDS - 1; i++) {
                deque_node_t previous, current;
                current = right_buffer[mod(i, DEF_BOUNDS)].load();
                previous = right_buffer[mod(i - 1, DEF_BOUNDS)].load();
                if(current.value == RNULL && previous.value != RNULL) {
                    head_found = true;
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = right_buffer;
            last_head = right_head;
            right_buffer = (atomic_deque_node_t *) right_buffer[DEF_BOUNDS - 1].load().value;
            right_head = i + 1;
            // switch to right check if next ptr is null
            if(right_buffer == RNULL)
                right_check = false;
        } else {
            // check over the current buffer
            for(i = right_head; mod(i, DEF_BOUNDS) > 0; i--) {
                deque_node_t next, current;
                current = right_buffer[mod(i, DEF_BOUNDS)].load();
                next = right_buffer[mod(i - 1, DEF_BOUNDS)].load();
                if(current.value == RNULL && next.value != RNULL) {
                    head_found = true;
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = right_buffer;
            last_head = right_head;
            right_buffer = (atomic_deque_node_t *) left_buffer[0].load().value;
            right_head = i - 2;
        }
    }
    assert(head_found);
    cindex = mod(last_head, DEF_BOUNDS);
    pindex = mod(last_head - 1, DEF_BOUNDS);
    fprintf(stdout, "\t\tRight head: %p @ %ld\n", last_buffer, last_head);
    fprintf(stdout, "\t\t\tCurrent: %p, Previous: %p\n", last_buffer[cindex].load().value, last_buffer[pindex].load().value);
    assert(last_buffer[cindex].load().value == RNULL);
    assert(last_buffer[pindex].load().value != RNULL);

    // all inserted elements are in deque
    // + no unknown elements are in deque
    int pop_status = OK;
    while(pop_status != EMPTY) {
        int *pp = left_pop(test_deque, pop_status);
        if(!is_null(pp) && pop_status == OK)
            found.insert(*pp);
    }
    for(set_iter = expected.begin(); set_iter != expected.end(); set_iter++)
        assert(found.find(*set_iter) != found.end());
    for(set_iter = found.begin(); set_iter != found.end(); set_iter++)
        assert(expected.find(*set_iter) != expected.end());
    
    return 0;
}

void *spec002_helper(void *args_void) {
    int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    //fprintf(stdout, "\t\tThread %d barrier %p\n", id, barrier);
    int i, push_status;

    wait_on_barrier(barrier);
    int *push_value = (int *) malloc(sizeof(int));
    *push_value = id + 1;
    right_push(test_deque, push_value, push_status);
    assert(push_status == OK);
    wait_on_barrier(barrier);
    
    pthread_exit(NULL);
}

//     02. two thread pools push right 64 times in total
int spec002() {
    int i;
    thread_pool_t pool1, pool2;
    thread_args_t targs1, targs2;
    pthread_barrier_t barrier;
    spec_result_t *results1, *results2;
    void **rvoid1, **rvoid2;
    set<int> expected, found;
    set<int>::iterator set_iter;
    vector<atomic_deque_node_t *> left_buffer_chain, right_buffer_chain;
    vector<atomic_deque_node_t *>::iterator left_iter;
    vector<atomic_deque_node_t *>::reverse_iterator right_iter;
    deque_hint_t left_hint, right_hint;
    atomic_deque_node_t *left_buffer, *right_buffer, *last_buffer;
    long int left_head, right_head, last_head;
    bool left_check, right_check, head_found, unreachable;
    
    fprintf(stdout, "Spec %3d\n", 2);
    for(i = 1; i <= THREAD_COUNT * 2; i++)
        expected.insert(i);
    init_pthread_barrier(barrier, THREAD_COUNT * 2);
    init_thread_pool(pool1, 1, THREAD_COUNT);
    init_thread_pool(pool2, 2, THREAD_COUNT);
    init_thread_args(targs1, 1, &barrier);
    init_thread_args(targs2, 2, &barrier);
    start_thread_pool(pool1, &spec002_helper, targs1);
    start_thread_pool(pool2, &spec002_helper, targs2);
    rvoid1 = wait_on_thread_pool(pool1);
    rvoid2 = wait_on_thread_pool(pool2);
    
    // both hints see exactly the same buffer chain
    left_hint = test_deque.left_hint.load();
    left_buffer = left_hint.nodes;
    left_head = left_hint.index;
    right_hint = test_deque.right_hint.load();
    right_buffer = right_hint.nodes;
    right_head = right_hint.index;
    atomic_deque_node_t *iter = left_buffer;
    while(iter[0].load().value != LNULL)
        iter = (atomic_deque_node_t *) iter[0].load().value;
    while(iter != (atomic_deque_node_t *) RNULL) {
        left_buffer_chain.push_back(iter);
        iter = (atomic_deque_node_t *) iter[DEF_BOUNDS - 1].load().value;
    }
    iter = right_buffer;
    while(iter[DEF_BOUNDS - 1].load().value != RNULL)
        iter = (atomic_deque_node_t *) iter[DEF_BOUNDS - 1].load().value;
    while(iter != (atomic_deque_node_t *) LNULL) {
        right_buffer_chain.push_back(iter);
        iter = (atomic_deque_node_t *) iter[0].load().value;
    }
    left_iter = left_buffer_chain.begin();
    right_iter = right_buffer_chain.rbegin();
    while(left_iter != left_buffer_chain.end() && right_iter != right_buffer_chain.rend()) {
        if(DEBUG) fprintf(stdout, "\tLeft chain: %p, right chain: %p\n", *left_iter, *right_iter);
        assert(*left_iter == *right_iter);
        left_iter++;
        right_iter++;
    }
    
    // actual heads are reachable from hints
    left_check = true;
    head_found = false;
    unreachable = false;
    while(!head_found) {
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
            last_buffer = left_buffer;
            last_head = left_head;
            left_buffer = (atomic_deque_node_t *) left_buffer[0].load().value;
            left_head = i - 1;
            // switch to right check if next ptr is null
            if(left_buffer == LNULL)
                left_check = false;
        } else {
            // check over the current buffer
            for(i = left_head; mod(i, DEF_BOUNDS) < DEF_BOUNDS - 1; i++) {
                deque_node_t next, current;
                current = left_buffer[mod(i, DEF_BOUNDS)].load();
                next = left_buffer[mod(i + 1, DEF_BOUNDS)].load();
                if(current.value == LNULL && next.value != LNULL) {
                    head_found = (next.value == RNULL);
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = left_buffer;
            last_head = left_head;
            left_buffer = (atomic_deque_node_t *) left_buffer[DEF_BOUNDS - 1].load().value;
            left_head = i + 2;
        }
    }
    assert(head_found);
    long int cindex = mod(last_head, DEF_BOUNDS), pindex = mod(last_head + 1, DEF_BOUNDS);
    fprintf(stdout, "\t\tLeft head: %p @ %ld\n", last_buffer, last_head);
    assert(last_buffer[cindex].load().value == LNULL);
    assert(last_buffer[pindex].load().value != LNULL);
    fprintf(stdout, "\t\t\tCurrent: %p, Previous: %p\n", last_buffer[cindex].load().value, last_buffer[pindex].load().value);
    
    right_check = true;
    head_found = false;
    unreachable = false;
    while(!head_found) {
        if(right_check) {
            // check over the current buffer
            for(i = right_head; mod(i, DEF_BOUNDS) <= DEF_BOUNDS - 1; i++) {
                deque_node_t previous, current;
                current = right_buffer[mod(i, DEF_BOUNDS)].load();
                previous = right_buffer[mod(i - 1, DEF_BOUNDS)].load();
                if(current.value == RNULL && previous.value != RNULL) {
                    head_found = true;
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = right_buffer;
            last_head = right_head;
            right_buffer = (atomic_deque_node_t *) right_buffer[DEF_BOUNDS - 1].load().value;
            right_head = i + 1;
            // switch to right check if next ptr is null
            if(right_buffer == RNULL)
                right_check = false;
        } else {
            // check over the current buffer
            for(i = right_head; mod(i, DEF_BOUNDS) > 0; i--) {
                deque_node_t next, current;
                current = right_buffer[mod(i, DEF_BOUNDS)].load();
                next = right_buffer[mod(i - 1, DEF_BOUNDS)].load();
                if(current.value == RNULL && next.value != RNULL) {
                    head_found = true;
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = right_buffer;
            last_head = right_head;
            right_buffer = (atomic_deque_node_t *) left_buffer[0].load().value;
            right_head = i - 2;
        }
    }
    assert(head_found);
    cindex = mod(last_head, DEF_BOUNDS);
    pindex = mod(last_head - 1, DEF_BOUNDS);
    fprintf(stdout, "\t\tRight head: %p @ %ld\n", last_buffer, last_head);
    fprintf(stdout, "\t\t\tCurrent: %p, Previous: %p\n", last_buffer[cindex].load().value, last_buffer[pindex].load().value);
    assert(last_buffer[cindex].load().value == RNULL);
    assert(last_buffer[pindex].load().value != RNULL);

    // all inserted elements are in deque
    // + no unknown elements are in deque
    int pop_status = OK;
    while(pop_status != EMPTY) {
        int *pp = left_pop(test_deque, pop_status);
        if(!is_null(pp) && pop_status == OK)
            found.insert(*pp);
    }
    for(set_iter = expected.begin(); set_iter != expected.end(); set_iter++)
        assert(found.find(*set_iter) != found.end());
    for(set_iter = found.begin(); set_iter != found.end(); set_iter++)
        assert(expected.find(*set_iter) != expected.end());
    
    return 0;
}

void *spec003_helper(void *args_void) {
    int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    spec_result_t *results = (spec_result_t *) malloc(sizeof(spec_result_t));
    //fprintf(stdout, "\t\tThread %d barrier %p\n", id, barrier);
    int i, pop_status;
    results->size = 1;
    results->results = (int **) malloc(sizeof(int *));

    wait_on_barrier(barrier);
    results->results[0] = left_pop(test_deque, pop_status);
    assert(pop_status == OK);
    assert(!is_null(results->results[0]));
    wait_on_barrier(barrier);
    
    pthread_exit((void *) results);
}

//     03. two thread pools pop left 64 times in total
int spec003() {
    int i, push_status;
    thread_pool_t pool1, pool2;
    thread_args_t targs1, targs2;
    pthread_barrier_t barrier;
    spec_result_t *results1, *results2;
    void **rvoid1, **rvoid2;
    set<int> expected, found;
    set<int>::iterator set_iter;
    vector<atomic_deque_node_t *> left_buffer_chain, right_buffer_chain;
    vector<atomic_deque_node_t *>::iterator left_iter;
    vector<atomic_deque_node_t *>::reverse_iterator right_iter;
    deque_hint_t left_hint, right_hint;
    atomic_deque_node_t *left_buffer, *right_buffer, *last_buffer;
    long int left_head, right_head, last_head;
    bool left_check, right_check, head_found, unreachable;
    
    fprintf(stdout, "Spec %3d\n", 3);
    for(i = 1; i <= THREAD_COUNT * 2; i += 2) {
        expected.insert(i);
        expected.insert(i + 1);
        int *left_val = (int *) malloc(sizeof(int));
        int *right_val = (int *) malloc(sizeof(int));
        *left_val = i;
        *right_val = i + 1;
        left_push(test_deque, left_val, push_status);
        assert(push_status == OK);
        right_push(test_deque, right_val, push_status);
        assert(push_status == OK);
    }
    init_pthread_barrier(barrier, THREAD_COUNT * 2);
    init_thread_pool(pool1, 1, THREAD_COUNT);
    init_thread_pool(pool2, 2, THREAD_COUNT);
    init_thread_args(targs1, 1, &barrier);
    init_thread_args(targs2, 2, &barrier);
    start_thread_pool(pool1, &spec003_helper, targs1);
    start_thread_pool(pool2, &spec003_helper, targs2);
    rvoid1 = wait_on_thread_pool(pool1);
    rvoid2 = wait_on_thread_pool(pool2);
    results1 = (spec_result_t *) malloc(sizeof(spec_result_t) * pool1.size);
    results2 = (spec_result_t *) malloc(sizeof(spec_result_t) * pool2.size);
    for(i = 0; i < THREAD_COUNT; i++) {
        if(rvoid1[i] != NULL) {
            results1[i].size = ((spec_result_t *) rvoid1[i])->size;
            results1[i].results = ((spec_result_t *) rvoid1[i])->results;
            free(rvoid1[i]);
        } else {
            results1[i].size = 0;
            results1[i].results = NULL;
        }
        if(results1[i].size > 0 && results1[i].results != NULL && !is_null(results1[i].results[0]))
            found.insert(*results1[i].results[0]);
        else if(is_null(results1[i].results[0]))
            fprintf(stdout, "\t\t\twtf? popped %p\n", results1[i].results[0]);
    }
    for(i = 0; i < THREAD_COUNT; i++) {
        if(rvoid2[i] != NULL) {
            results2[i].size = ((spec_result_t *) rvoid2[i])->size;
            results2[i].results = ((spec_result_t *) rvoid2[i])->results;
            free(rvoid2[i]);
        } else {
            results2[i].size = 0;
            results2[i].results = NULL;
        }
        if(results2[i].size > 0 && results2[i].results != NULL && !is_null(results2[i].results[0]))
            found.insert(*results2[i].results[0]);
        else if(is_null(results2[i].results[0]))
            fprintf(stdout, "\t\t\twtf? popped %p\n", results2[i].results[0]);
    }
    
    // both hints see exactly the same buffer chain
    left_hint = test_deque.left_hint.load();
    left_buffer = left_hint.nodes;
    left_head = left_hint.index;
    right_hint = test_deque.right_hint.load();
    right_buffer = right_hint.nodes;
    right_head = right_hint.index;
    atomic_deque_node_t *iter = left_buffer;
    while(iter[0].load().value != LNULL)
        iter = (atomic_deque_node_t *) iter[0].load().value;
    while(iter != (atomic_deque_node_t *) RNULL) {
        left_buffer_chain.push_back(iter);
        iter = (atomic_deque_node_t *) iter[DEF_BOUNDS - 1].load().value;
    }
    iter = right_buffer;
    while(iter[DEF_BOUNDS - 1].load().value != RNULL)
        iter = (atomic_deque_node_t *) iter[DEF_BOUNDS - 1].load().value;
    while(iter != (atomic_deque_node_t *) LNULL) {
        right_buffer_chain.push_back(iter);
        iter = (atomic_deque_node_t *) iter[0].load().value;
    }
    left_iter = left_buffer_chain.begin();
    right_iter = right_buffer_chain.rbegin();
    while(left_iter != left_buffer_chain.end() && right_iter != right_buffer_chain.rend()) {
        if(DEBUG) fprintf(stdout, "\tLeft chain: %p, right chain: %p\n", *left_iter, *right_iter);
        assert(*left_iter == *right_iter);
        left_iter++;
        right_iter++;
    }
    
    // actual heads are reachable from hints
    left_check = true;
    head_found = false;
    unreachable = false;
    while(!head_found) {
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
            last_buffer = left_buffer;
            last_head = left_head;
            left_buffer = (atomic_deque_node_t *) left_buffer[0].load().value;
            left_head = i - 1;
            // switch to right check if next ptr is null
            if(left_buffer == LNULL)
                left_check = false;
        } else {
            // check over the current buffer
            for(i = left_head; mod(i, DEF_BOUNDS) < DEF_BOUNDS - 1; i++) {
                deque_node_t next, current;
                current = left_buffer[mod(i, DEF_BOUNDS)].load();
                next = left_buffer[mod(i + 1, DEF_BOUNDS)].load();
                if(current.value == LNULL && next.value != LNULL) {
                    head_found = (next.value == RNULL);
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = left_buffer;
            last_head = left_head;
            left_buffer = (atomic_deque_node_t *) left_buffer[DEF_BOUNDS - 1].load().value;
            left_head = i + 2;
        }
    }
    assert(head_found);
    long int cindex = mod(last_head, DEF_BOUNDS), pindex = mod(last_head + 1, DEF_BOUNDS);
    fprintf(stdout, "\t\tLeft head: %p @ %ld\n", last_buffer, last_head);
    assert(last_buffer[cindex].load().value == LNULL);
    assert(last_buffer[pindex].load().value != LNULL);
    fprintf(stdout, "\t\t\tCurrent: %p, Previous: %p\n", last_buffer[cindex].load().value, last_buffer[pindex].load().value);
    
    right_check = true;
    head_found = false;
    unreachable = false;
    while(!head_found) {
        if(right_check) {
            // check over the current buffer
            for(i = right_head; mod(i, DEF_BOUNDS) <= DEF_BOUNDS - 1; i++) {
                deque_node_t previous, current;
                current = right_buffer[mod(i, DEF_BOUNDS)].load();
                previous = right_buffer[mod(i - 1, DEF_BOUNDS)].load();
                if(current.value == RNULL && previous.value != RNULL) {
                    head_found = true;
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = right_buffer;
            last_head = right_head;
            right_buffer = (atomic_deque_node_t *) right_buffer[DEF_BOUNDS - 1].load().value;
            right_head = i + 1;
            // switch to right check if next ptr is null
            if(right_buffer == RNULL)
                right_check = false;
        } else {
            // check over the current buffer
            for(i = right_head; mod(i, DEF_BOUNDS) > 0; i--) {
                deque_node_t next, current;
                current = right_buffer[mod(i, DEF_BOUNDS)].load();
                next = right_buffer[mod(i - 1, DEF_BOUNDS)].load();
                if(current.value == RNULL && next.value != RNULL) {
                    head_found = true;
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = right_buffer;
            last_head = right_head;
            right_buffer = (atomic_deque_node_t *) left_buffer[0].load().value;
            right_head = i - 2;
        }
    }
    assert(head_found);
    cindex = mod(last_head, DEF_BOUNDS);
    pindex = mod(last_head - 1, DEF_BOUNDS);
    fprintf(stdout, "\t\tRight head: %p @ %ld\n", last_buffer, last_head);
    fprintf(stdout, "\t\t\tCurrent: %p, Previous: %p\n", last_buffer[cindex].load().value, last_buffer[pindex].load().value);
    assert(last_buffer[cindex].load().value == RNULL);
    assert(last_buffer[pindex].load().value != RNULL);

    // all inserted elements are in deque
    // + no unknown elements are in deque
    int pop_status = OK;
    left_pop(test_deque, pop_status);
    assert(pop_status == EMPTY);
    right_pop(test_deque, pop_status);
    assert(pop_status == EMPTY);
    for(set_iter = expected.begin(); set_iter != expected.end(); set_iter++)
        assert(found.find(*set_iter) != found.end());
    for(set_iter = found.begin(); set_iter != found.end(); set_iter++)
        assert(expected.find(*set_iter) != expected.end());
    
    return 0;
}

void *spec004_helper(void *args_void) {
    int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    spec_result_t *results = (spec_result_t *) malloc(sizeof(spec_result_t));
    //fprintf(stdout, "\t\tThread %d barrier %p\n", id, barrier);
    int i, pop_status;
    results->size = 1;
    results->results = (int **) malloc(sizeof(int *));

    wait_on_barrier(barrier);
    results->results[0] = right_pop(test_deque, pop_status);
    assert(pop_status == OK);
    assert(!is_null(results->results[0]));
    wait_on_barrier(barrier);
    
    pthread_exit((void *) results);
}

//     04. two thread pools pop left 64 times in total
int spec004() {
    int i, push_status;
    thread_pool_t pool1, pool2;
    thread_args_t targs1, targs2;
    pthread_barrier_t barrier;
    spec_result_t *results1, *results2;
    void **rvoid1, **rvoid2;
    set<int> expected, found;
    set<int>::iterator set_iter;
    vector<atomic_deque_node_t *> left_buffer_chain, right_buffer_chain;
    vector<atomic_deque_node_t *>::iterator left_iter;
    vector<atomic_deque_node_t *>::reverse_iterator right_iter;
    deque_hint_t left_hint, right_hint;
    atomic_deque_node_t *left_buffer, *right_buffer, *last_buffer;
    long int left_head, right_head, last_head;
    bool left_check, right_check, head_found, unreachable;
    
    fprintf(stdout, "Spec %3d\n", 4);
    for(i = 1; i <= THREAD_COUNT * 2; i += 2) {
        expected.insert(i);
        expected.insert(i + 1);
        int *left_val = (int *) malloc(sizeof(int));
        int *right_val = (int *) malloc(sizeof(int));
        *left_val = i;
        *right_val = i + 1;
        left_push(test_deque, left_val, push_status);
        assert(push_status == OK);
        right_push(test_deque, right_val, push_status);
        assert(push_status == OK);
    }
    init_pthread_barrier(barrier, THREAD_COUNT * 2);
    init_thread_pool(pool1, 1, THREAD_COUNT);
    init_thread_pool(pool2, 2, THREAD_COUNT);
    init_thread_args(targs1, 1, &barrier);
    init_thread_args(targs2, 2, &barrier);
    start_thread_pool(pool1, &spec004_helper, targs1);
    start_thread_pool(pool2, &spec004_helper, targs2);
    rvoid1 = wait_on_thread_pool(pool1);
    rvoid2 = wait_on_thread_pool(pool2);
    results1 = (spec_result_t *) malloc(sizeof(spec_result_t) * pool1.size);
    results2 = (spec_result_t *) malloc(sizeof(spec_result_t) * pool2.size);
    for(i = 0; i < THREAD_COUNT; i++) {
        if(rvoid1[i] != NULL) {
            results1[i].size = ((spec_result_t *) rvoid1[i])->size;
            results1[i].results = ((spec_result_t *) rvoid1[i])->results;
            free(rvoid1[i]);
        } else {
            results1[i].size = 0;
            results1[i].results = NULL;
        }
        if(results1[i].size > 0 && results1[i].results != NULL && !is_null(results1[i].results[0]))
            found.insert(*results1[i].results[0]);
        else if(is_null(results1[i].results[0]))
            fprintf(stdout, "\t\t\twtf? popped %p\n", results1[i].results[0]);
    }
    for(i = 0; i < THREAD_COUNT; i++) {
        if(rvoid2[i] != NULL) {
            results2[i].size = ((spec_result_t *) rvoid2[i])->size;
            results2[i].results = ((spec_result_t *) rvoid2[i])->results;
            free(rvoid2[i]);
        } else {
            results2[i].size = 0;
            results2[i].results = NULL;
        }
        if(results2[i].size > 0 && results2[i].results != NULL && !is_null(results2[i].results[0]))
            found.insert(*results2[i].results[0]);
        else if(is_null(results2[i].results[0]))
            fprintf(stdout, "\t\t\twtf? popped %p\n", results2[i].results[0]);
    }
    
    // both hints see exactly the same buffer chain
    left_hint = test_deque.left_hint.load();
    left_buffer = left_hint.nodes;
    left_head = left_hint.index;
    right_hint = test_deque.right_hint.load();
    right_buffer = right_hint.nodes;
    right_head = right_hint.index;
    atomic_deque_node_t *iter = left_buffer;
    while(iter[0].load().value != LNULL)
        iter = (atomic_deque_node_t *) iter[0].load().value;
    while(iter != (atomic_deque_node_t *) RNULL) {
        left_buffer_chain.push_back(iter);
        iter = (atomic_deque_node_t *) iter[DEF_BOUNDS - 1].load().value;
    }
    iter = right_buffer;
    while(iter[DEF_BOUNDS - 1].load().value != RNULL)
        iter = (atomic_deque_node_t *) iter[DEF_BOUNDS - 1].load().value;
    while(iter != (atomic_deque_node_t *) LNULL) {
        right_buffer_chain.push_back(iter);
        iter = (atomic_deque_node_t *) iter[0].load().value;
    }
    left_iter = left_buffer_chain.begin();
    right_iter = right_buffer_chain.rbegin();
    while(left_iter != left_buffer_chain.end() && right_iter != right_buffer_chain.rend()) {
        if(DEBUG) fprintf(stdout, "\tLeft chain: %p, right chain: %p\n", *left_iter, *right_iter);
        assert(*left_iter == *right_iter);
        left_iter++;
        right_iter++;
    }
    
    // actual heads are reachable from hints
    left_check = true;
    head_found = false;
    unreachable = false;
    while(!head_found) {
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
            last_buffer = left_buffer;
            last_head = left_head;
            left_buffer = (atomic_deque_node_t *) left_buffer[0].load().value;
            left_head = i - 1;
            // switch to right check if next ptr is null
            if(left_buffer == LNULL)
                left_check = false;
        } else {
            // check over the current buffer
            for(i = left_head; mod(i, DEF_BOUNDS) < DEF_BOUNDS - 1; i++) {
                deque_node_t next, current;
                current = left_buffer[mod(i, DEF_BOUNDS)].load();
                next = left_buffer[mod(i + 1, DEF_BOUNDS)].load();
                if(current.value == LNULL && next.value != LNULL) {
                    head_found = (next.value == RNULL);
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = left_buffer;
            last_head = left_head;
            left_buffer = (atomic_deque_node_t *) left_buffer[DEF_BOUNDS - 1].load().value;
            left_head = i + 2;
        }
    }
    assert(head_found);
    long int cindex = mod(last_head, DEF_BOUNDS), pindex = mod(last_head + 1, DEF_BOUNDS);
    fprintf(stdout, "\t\tLeft head: %p @ %ld\n", last_buffer, last_head);
    assert(last_buffer[cindex].load().value == LNULL);
    assert(last_buffer[pindex].load().value != LNULL);
    fprintf(stdout, "\t\t\tCurrent: %p, Previous: %p\n", last_buffer[cindex].load().value, last_buffer[pindex].load().value);
    
    right_check = true;
    head_found = false;
    unreachable = false;
    while(!head_found) {
        if(right_check) {
            // check over the current buffer
            for(i = right_head; mod(i, DEF_BOUNDS) <= DEF_BOUNDS - 1; i++) {
                deque_node_t previous, current;
                current = right_buffer[mod(i, DEF_BOUNDS)].load();
                previous = right_buffer[mod(i - 1, DEF_BOUNDS)].load();
                if(current.value == RNULL && previous.value != RNULL) {
                    head_found = true;
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = right_buffer;
            last_head = right_head;
            right_buffer = (atomic_deque_node_t *) right_buffer[DEF_BOUNDS - 1].load().value;
            right_head = i + 1;
            // switch to right check if next ptr is null
            if(right_buffer == RNULL)
                right_check = false;
        } else {
            // check over the current buffer
            for(i = right_head; mod(i, DEF_BOUNDS) > 0; i--) {
                deque_node_t next, current;
                current = right_buffer[mod(i, DEF_BOUNDS)].load();
                next = right_buffer[mod(i - 1, DEF_BOUNDS)].load();
                if(current.value == RNULL && next.value != RNULL) {
                    head_found = true;
                    break;
                }
            }
            // move to next buffer if possible
            last_buffer = right_buffer;
            last_head = right_head;
            right_buffer = (atomic_deque_node_t *) left_buffer[0].load().value;
            right_head = i - 2;
        }
    }
    assert(head_found);
    cindex = mod(last_head, DEF_BOUNDS);
    pindex = mod(last_head - 1, DEF_BOUNDS);
    fprintf(stdout, "\t\tRight head: %p @ %ld\n", last_buffer, last_head);
    fprintf(stdout, "\t\t\tCurrent: %p, Previous: %p\n", last_buffer[cindex].load().value, last_buffer[pindex].load().value);
    assert(last_buffer[cindex].load().value == RNULL);
    assert(last_buffer[pindex].load().value != RNULL);

    // all inserted elements are in deque
    // + no unknown elements are in deque
    int pop_status = OK;
    left_pop(test_deque, pop_status);
    assert(pop_status == EMPTY);
    right_pop(test_deque, pop_status);
    assert(pop_status == EMPTY);
    for(set_iter = expected.begin(); set_iter != expected.end(); set_iter++)
        assert(found.find(*set_iter) != found.end());
    for(set_iter = found.begin(); set_iter != found.end(); set_iter++)
        assert(expected.find(*set_iter) != expected.end());
    
    return 0;
}

int spec005() {

}

// TODO: init these in main spec
/*atomic_int counter;
pthread_mutex_t vector_guard;
vector<int> vector_006;*/

void *spec006_helper(void *args_void) {
    /*int id = ((thread_args_t *) args_void)->id;
    pthread_barrier_t *barrier = ((thread_args_t *) args_void)->barrier;
    spec_result_t *results = (spec_result_t *) malloc(sizeof(spec_result_t));
    //fprintf(stdout, "\t\tThread %d barrier %p\n", id, barrier);
    int i, push_status, pop_status;
    
    wait_on_barrier(barrier);
    if(id >= 0 && id < THREAD_COUNT) {
        int *pval = (int *) malloc(sizeof(int));
        *pval = c.load();
        if(counter.compare_exchange_strong(*pval, *pval + 1)) {
            left_push(test_deque, pval, push_status);
        } else {
            free(pval);
        }
    } else {
        int *pval = left_pop(test_deque, pop_status);
        if(pop_status != EMPTY) {
            pthread_mutex_lock(&vector_guard);
            assert(pop_status == OK);
            vector_006.push_back(*pval);
            pthread_mutex_unlock(&vector_guard);
        }
    }
    wait_on_barrier(barrier);
    */
    pthread_exit(NULL);
}

//     06. one thread pool pushes left 64 times, one thread pool keeps popping right
int spec006() {
    /*int i, push_status;
    thread_pool_t pool1, pool2;
    thread_args_t targs1, targs2;
    pthread_barrier_t barrier;
    deque<int> expected, found;
    deque<int>::iterator v;
    void *rvoid1, *rvoid2;
    
    fprintf(stdout, "Spec %3d\n", 6);
    for(i = 1; i <= THREAD_COUNT * 2; i++)
        expected.push_back(i);
    init_pthread_barrier(barrier, THREAD_COUNT * 2);
    init_thread_pool(pool1, 1, THREAD_COUNT);
    init_thread_pool(pool2, 2, THREAD_COUNT);
    init_thread_args(targs1, 1, &barrier);
    init_thread_args(targs2, 2, &barrier);
    start_thread_pool(pool1, &spec006_helper, targs1);
    start_thread_pool(pool2, &spec006_helper, targs2);
    
    // all inserted elements are in deque
    // + no unknown elements are in deque
    int pop_status = OK;
    left_pop(test_deque, pop_status);
    assert(pop_status == EMPTY);
    right_pop(test_deque, pop_status);
    assert(pop_status == EMPTY);
    for(set_iter = expected.begin(); set_iter != expected.end(); set_iter++)
        assert(found.find(*set_iter) != found.end());
    for(set_iter = found.begin(); set_iter != found.end(); set_iter++)
        assert(expected.find(*set_iter) != expected.end());
    */
    return 0;
}

int spec007() {

}

int main(int argc, char *argv[]) {
    int (*specs[SPEC_COUNT])(void) = { &spec001, &spec002, &spec003, &spec004, &spec005, &spec006, &spec007 };
    int i, spec_stat;
    
    for(i = 0; i < SPEC_COUNT; i++) {
        init_deque(test_deque);
        spec_stat = specs[i]();
        clear_deque(test_deque);
    }
    
    return 0;
}
