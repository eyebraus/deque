
#include <atomic>
#include <stdio.h>
#include <stdlib.h>
#include "deque.h"
#include "spec.h"

#define SPEC_COUNT 1

using namespace std;

bounded_deque_t test_deque;

/*
 * Specs:
 *     1. push one element, then pop it
 *     ...
 */
int spec_001(int &stat) {
    
}

int main(int argc, char *argv[]) {
    init_bounded_deque(test_deque);
    int (*specs)(int &stat)[SPEC_COUNT] = { &spec001 };
    int i, spec_stat;
    
    for(i = 0; i < SPEC_COUNT; i++) {
        specs[i](spec_stat);
        
        clear_bounded_deque(test_deque);
    }
    
    return 0;
}

