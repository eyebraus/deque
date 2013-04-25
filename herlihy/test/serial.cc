
#include <assert.h>
#include <atomic>
#include <stdio.h>
#include <stdlib.h>
#include "../src/deque.h"
#include "spec.h"

#define SPEC_COUNT 10

using namespace std;

bounded_deque_t test_deque;

/*
 * Specs:
 *     01 and 02. push one element, then pop it (left and right)
 *     03 and 04. push until full, the pop until empty (left and right)
 *     05. push several to left, then pop from right until empty
 *     06. push several to right, then pop from left until empty
 *     07. consistent initialization state
 *     08. pop empty queue
 *     09. push full queue
 *     10. check counters on nodes
 *     ...
 */

int spec007(int &stat) {
    fprintf(stdout, "Spec %3d\n", 7);
    int i;
    assert(test_deque.size == 0);
    assert(test_deque.left_hint == (DEF_BOUNDS / 2 - 1));
    assert(test_deque.right_hint == DEF_BOUNDS / 2);
    for(i = 0; i < DEF_BOUNDS; i++) {
        fprintf(stdout, "\tIteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(test_deque.nodes[i].load().value == LNULL);
        else
            assert(test_deque.nodes[i].load().value == RNULL);
        assert(test_deque.nodes[i].load().count == 0);
    }
    return 0;
}

int spec008(int &stat) {
    fprintf(stdout, "Spec %3d\n", 8);
    fprintf(stdout, "\tLeft empty pop\n");
    int pop_status = 0, *pop_value;
    pop_value = left_pop(test_deque, pop_status);
    assert(pop_value == NULL);
    assert(pop_status == EMPTY);
    fprintf(stdout, "\tRight empty pop\n");
    pop_value = right_pop(test_deque, pop_status);
    assert(pop_value == NULL);
    assert(pop_status == EMPTY);
    return 0;
}

int spec009(int &stat) {
    fprintf(stdout, "Spec %3d\n", 9);
    int i = 0, push_status = 0, push_value = 0xbeef;
    unsigned long int old_size = 0, size_diff = 0;
    unsigned long int half_cap = DEF_BOUNDS / 2 - 1;
    // fill everything on the left
    fprintf(stdout, "\tLeft push til full test\n");
    while(true) {
        fprintf(stdout, "\t\tPush iteration %3d\n", i++);
        if(test_deque.size >= half_cap + old_size) {
            old_size = test_deque.size;
            break;
        }
        left_push(test_deque, &push_value, push_status);
        assert(push_status == OK);
    }
    // make sure we can't push now
    left_push(test_deque, &push_value, push_status);
    assert(test_deque.size == old_size);
    assert(push_status == FULL);
    // fill everything on the right
    fprintf(stdout, "\tRight push til full test\n");
    i = 0;
    while(true) {
        fprintf(stdout, "\t\tPush iteration %3d\n", i++);
        if(test_deque.size >= half_cap + old_size) {
            old_size = test_deque.size;
            break;
        }
        right_push(test_deque, &push_value, push_status);
        assert(push_status == OK);
    }
    // make sure we can't push now
    right_push(test_deque, &push_value, push_status);
    assert(test_deque.size == old_size);
    assert(push_status == FULL);
    return 0;
}

int spec001(int &stat) {
    fprintf(stdout, "Spec %3d\n", 1);
    int push_status = 0, pop_status = 0;
    int push_value = 5, pop_value;
    left_push(test_deque, &push_value, push_status);
    assert(push_status == OK);
    assert(test_deque.size == 1);
    pop_value = *left_pop(test_deque, pop_status);
    assert(pop_status == 0);
    assert(test_deque.size == 0);
    assert(push_value == pop_value);
    return 0;
}

int spec002(int &stat) {
    fprintf(stdout, "Spec %3d\n", 2);
    int push_status = 0, pop_status = 0;
    int push_value = 5, pop_value;
    right_push(test_deque, &push_value, push_status);
    assert(push_status == OK);
    assert(test_deque.size == 1);
    pop_value = *right_pop(test_deque, pop_status);
    assert(pop_status == 0);
    assert(test_deque.size == 0);
    assert(push_value == pop_value);
    return 0;
}

int spec003(int &stat) {
    fprintf(stdout, "Spec %3d\n", 3);
    int i, e = 0, *push_value, push_status = 0, pop_status = 0, pop_value;
    unsigned long int half_cap = DEF_BOUNDS / 2 - 1;
    // fill to the left
    while(true) {
        if(test_deque.size >= half_cap)
            break;
        fprintf(stdout, "\tPush iteration %3d\n", e);
        push_value = (int *) malloc(sizeof(int));
        *push_value = e++;
        left_push(test_deque, push_value, push_status);
        assert(test_deque.size == e);
        assert(push_status == OK);
    }
    assert(test_deque.size == (DEF_BOUNDS / 2 - 1));
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(!is_null(test_deque.nodes[i].load().value));
        else
            assert(test_deque.nodes[i].load().value == RNULL);
    }
    // remove all from the left
    while(true) {
        if(test_deque.size <= 0)
            break;
        fprintf(stdout, "\tPop iteration %3d\n", e);
        pop_value = *left_pop(test_deque, pop_status);
        assert(pop_value == --e);
        assert(test_deque.size == e);
        assert(pop_status == OK);
    }
    assert(test_deque.size == 0);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(test_deque.nodes[i].load().value == LNULL);
        else
            assert(test_deque.nodes[i].load().value == RNULL);
    }
    return 0;
}

int spec004(int &stat) {
    fprintf(stdout, "Spec %3d\n", 4);
    int i, e = 0, *push_value, push_status = 0, pop_status = 0, pop_value;
    unsigned long int half_cap = DEF_BOUNDS / 2 - 1;
    // fill to the right
    while(true) {
        if(test_deque.size >= half_cap)
            break;
        fprintf(stdout, "\tPush iteration %3d\n", e);
        push_value = (int *) malloc(sizeof(int));
        *push_value = e++;
        right_push(test_deque, push_value, push_status);
        assert(test_deque.size == e);
        assert(push_status == OK);
    }
    assert(test_deque.size == (DEF_BOUNDS / 2 - 1));
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(test_deque.nodes[i].load().value == LNULL);
        else
            assert(!is_null(test_deque.nodes[i].load().value));
    }
    // remove all from the right
    while(true) {
        if(test_deque.size <= 0)
            break;
        fprintf(stdout, "\tPop iteration %3d\n", e);
        pop_value = *right_pop(test_deque, pop_status);
        assert(pop_value == --e);
        assert(test_deque.size == e);
        assert(pop_status == OK);
    }
    assert(test_deque.size == 0);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(test_deque.nodes[i].load().value == LNULL);
        else
            assert(test_deque.nodes[i].load().value == RNULL);
    }
    return 0;
}

int spec005(int &stat) {
    fprintf(stdout, "Spec %3d\n", 5);
    int i, e = 0, *push_value, push_status = 0, pop_status = 0, pop_value;
    unsigned long int half_cap = DEF_BOUNDS / 2 - 1;
    // fill to the left
    while(true) {
        if(test_deque.size >= half_cap)
            break;
        fprintf(stdout, "\tPush iteration %3d\n", e);
        push_value = (int *) malloc(sizeof(int));
        *push_value = e++;
        left_push(test_deque, push_value, push_status);
        assert(test_deque.size == e);
        assert(push_status == OK);
    }
    assert(test_deque.size == (DEF_BOUNDS / 2 - 1));
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(!is_null(test_deque.nodes[i].load().value));
        else
            assert(test_deque.nodes[i].load().value == RNULL);
    }
    // remove all from the right
    e = 0;
    while(true) {
        if(test_deque.size <= 0)
            break;
        fprintf(stdout, "\tPop iteration %3d\n", e);
        pop_value = *right_pop(test_deque, pop_status);
        assert(pop_value == e++);
        assert(test_deque.size == (half_cap - e));
        assert(pop_status == OK);
    }
    assert(test_deque.size == 0);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        assert(test_deque.nodes[i].load().value == RNULL);
    }
    return 0;
}

int spec006(int &stat) {
    fprintf(stdout, "Spec %3d\n", 6);
    int i, e = 0, *push_value, push_status = 0, pop_status = 0, pop_value;
    unsigned long int half_cap = DEF_BOUNDS / 2 - 1;
    // fill to the right
    while(true) {
        if(test_deque.size >= half_cap)
            break;
        fprintf(stdout, "\tPush iteration %3d\n", e);
        push_value = (int *) malloc(sizeof(int));
        *push_value = e++;
        right_push(test_deque, push_value, push_status);
        assert(test_deque.size == e);
        assert(push_status == OK);
    }
    assert(test_deque.size == (DEF_BOUNDS / 2 - 1));
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(test_deque.nodes[i].load().value == LNULL);
        else
            assert(!is_null(test_deque.nodes[i].load().value));
    }
    // remove all from the left
    e = 0;
    while(true) {
        if(test_deque.size <= 0)
            break;
        fprintf(stdout, "\tPop iteration %3d\n", e);
        pop_value = *left_pop(test_deque, pop_status);
        assert(pop_value == e++);
        assert(test_deque.size == (half_cap - e));
        assert(pop_status == OK);
    }
    assert(test_deque.size == 0);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        assert(test_deque.nodes[i].load().value == LNULL);
    }
    return 0;
}

int spec010(int &stat) {
    fprintf(stdout, "Spec %3d\n", 10);
    int i;
    int push_status = 0, pop_status = 0;
    int push_value = 0xbeef, pop_value;
    // add three, pop two, push one, pop two
    right_push(test_deque, &push_value, push_status);
    right_push(test_deque, &push_value, push_status);
    right_push(test_deque, &push_value, push_status);
    pop_value = *left_pop(test_deque, pop_status);
    pop_value = *left_pop(test_deque, pop_status);
    left_push(test_deque, &push_value, push_status);
    pop_value = *left_pop(test_deque, pop_status);
    pop_value = *left_pop(test_deque, pop_status);
    // check all counters
    for(i = 0; i < DEF_BOUNDS; i++) {
        fprintf(stdout, "\tCheck iteration %3d\n", i);
        if(i == DEF_BOUNDS / 2 - 1) {
            assert(test_deque.nodes[i].load().count == 2);
        } else if(i == DEF_BOUNDS / 2) {
            assert(test_deque.nodes[i].load().count == 5);
        } else if(i == DEF_BOUNDS / 2 + 1) {
            assert(test_deque.nodes[i].load().count == 6);
        } else if(i == DEF_BOUNDS / 2 + 2) {
            assert(test_deque.nodes[i].load().count == 3);
        } else {
            assert(test_deque.nodes[i].load().count == 0);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int (*specs[SPEC_COUNT])(int &stat) = { &spec007, &spec008, &spec009, &spec001, &spec002, &spec003, &spec004, &spec005, &spec006, &spec010 };
    int i, spec_stat;
    
    for(i = 0; i < SPEC_COUNT; i++) {
        init_bounded_deque(test_deque);
        specs[i](spec_stat);
        clear_bounded_deque(test_deque);
    }
    
    return 0;
}

