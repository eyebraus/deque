
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

#define SPEC_COUNT 7

using namespace std;

deque_t test_deque;

typedef struct spec_result_struct {
    int size;
    int **results;
} spec_result_t;

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

int spec001() {

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
