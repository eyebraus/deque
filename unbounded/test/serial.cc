
#include <assert.h>
#include <atomic>
#include <stdio.h>
#include <stdlib.h>
#include "../src/deque.h"
#include "spec.h"

#define SPEC_COUNT 16

using namespace std;

deque_t test_deque;

/*
 * Specs:
 *     01 and 02. push one element, then pop it (left and right)
 *     03 and 04. push to buffer limit, the pop until empty (left and right)
 *     05. push several to left, then pop from right until empty
 *     06. push several to right, then pop from left until empty
 *     07. consistent initialization state
 *     XX. pop empty queue
 *     XX. push full queue
 *     XX. check counters on nodes
 *     11 and 12. expand into a new buffer then pop away from it (left and right)
 *     13 and 14. expand into a new buffer then pop into it (left and right)
 *     15 and 16. check emptiness in the straddling case (left and right)
 *     ...
 */

int spec007(int &stat) {
    fprintf(stdout, "Spec %3d\n", 7);
    int i;
    assert(test_deque.size == 0);
    //assert(test_deque.left_hint == (DEF_BOUNDS / 2 - 1));
    //assert(test_deque.right_hint == DEF_BOUNDS / 2);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 - 1);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2);
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    for(i = 0; i < DEF_BOUNDS; i++) {
        fprintf(stdout, "\tIteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(test_deque.left_hint.load().nodes[i].load().value == LNULL);
        else
            assert(test_deque.left_hint.load().nodes[i].load().value == RNULL);
        assert(test_deque.left_hint.load().nodes[i].load().count == 0);
    }
    return 0;
}

int spec008(int &stat) {
    /*fprintf(stdout, "Spec %3d\n", 8);
    fprintf(stdout, "\tLeft empty pop\n");
    int pop_status = 0, *pop_value;
    pop_value = left_pop(test_deque, pop_status);
    assert(pop_value == NULL);
    assert(pop_status == EMPTY);
    fprintf(stdout, "\tRight empty pop\n");
    pop_value = right_pop(test_deque, pop_status);
    assert(pop_value == NULL);
    assert(pop_status == EMPTY);*/
    return 0;
}

int spec009(int &stat) {
    /*fprintf(stdout, "Spec %3d\n", 9);
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
    assert(push_status == FULL);*/
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
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(!is_null(test_deque.left_hint.load().nodes[i].load().value));
        else
            assert(test_deque.left_hint.load().nodes[i].load().value == RNULL);
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
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(test_deque.left_hint.load().nodes[i].load().value == LNULL);
        else
            assert(test_deque.left_hint.load().nodes[i].load().value == RNULL);
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
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(test_deque.left_hint.load().nodes[i].load().value == LNULL);
        else
            assert(!is_null(test_deque.left_hint.load().nodes[i].load().value));
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
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(test_deque.left_hint.load().nodes[i].load().value == LNULL);
        else
            assert(test_deque.left_hint.load().nodes[i].load().value == RNULL);
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
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(!is_null(test_deque.left_hint.load().nodes[i].load().value));
        else
            assert(test_deque.left_hint.load().nodes[i].load().value == RNULL);
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
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        assert(test_deque.left_hint.load().nodes[i].load().value == RNULL);
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
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        if(i < DEF_BOUNDS / 2)
            assert(test_deque.left_hint.load().nodes[i].load().value == LNULL);
        else
            assert(!is_null(test_deque.left_hint.load().nodes[i].load().value));
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
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    // afterward, verify expected deque state
    for(i = 1; i < DEF_BOUNDS - 1; i++) {
        fprintf(stdout, "\t\tCheck iteration %3d\n", i);
        assert(test_deque.left_hint.load().nodes[i].load().value == LNULL);
    }
    return 0;
}

int spec010(int &stat) {
    /*fprintf(stdout, "Spec %3d\n", 10);
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
    }*/
    return 0;
}

//     11 and 12. expand into a new buffer (left and right)
int spec011(int &stat) {
    fprintf(stdout, "Spec %3d\n", 11);
    int e = 0, push_status, pop_status, *push_value, pop_value;
    int cap = DEF_BOUNDS / 2;
    // push all to left
    while(true) {
        if(test_deque.size >= cap)
            break;
        fprintf(stdout, "\tPush iteration %3d\n", e);
        push_value = (int *) malloc(sizeof(int));
        *push_value = e++;
        left_push(test_deque, push_value, push_status);
        assert(test_deque.size == e);
        assert(push_status == OK);
    }
    // push over half-cap, then make sure hints changed
    assert(test_deque.left_hint.load().nodes != test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == -3);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2);
    assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == (void *) test_deque.right_hint.load().nodes);
    assert((void *) test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes[0].load().value);
    // pop all from left
    while(true) {
        if(test_deque.size <= 0)
            break;
        fprintf(stdout, "\tPop iteration %3d\n", e);
        pop_value = *left_pop(test_deque, pop_status);
        assert(pop_value == --e);
        assert(test_deque.size == e);
        assert(pop_status == OK);
    }
    // deque all, then make sure hints changed
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 - 1);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2);
    //assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == RNULL);
    //assert(test_deque.right_hint.load().nodes[0].load().value == LNULL);
    return 0;
}

int spec012(int &stat) {
    fprintf(stdout, "Spec %3d\n", 12);
    int e = 0, push_status, pop_status, *push_value, pop_value;
    int cap = DEF_BOUNDS / 2;
    // push all to left
    while(true) {
        if(test_deque.size >= cap)
            break;
        fprintf(stdout, "\tPush iteration %3d\n", e);
        push_value = (int *) malloc(sizeof(int));
        *push_value = e++;
        right_push(test_deque, push_value, push_status);
        assert(test_deque.size == e);
        assert(push_status == OK);
    }
    // push over half-cap, then make sure hints changed
    assert(test_deque.left_hint.load().nodes != test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 - 1);
    assert(test_deque.right_hint.load().index == 34);
    assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == (void *) test_deque.right_hint.load().nodes);
    assert((void *) test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes[0].load().value);
    // pop all from left
    while(true) {
        if(test_deque.size <= 0)
            break;
        fprintf(stdout, "\tPop iteration %3d\n", e);
        pop_value = *right_pop(test_deque, pop_status);
        assert(pop_value == --e);
        assert(test_deque.size == e);
        assert(pop_status == OK);
    }
    // deque all, then make sure hints changed
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 - 1);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2);
    //assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == RNULL);
    //assert(test_deque.right_hint.load().nodes[0].load().value == LNULL);
    return 0;
}

//     13 and 14. expand into a new buffer then pop into it (left and right)
int spec013(int &stat) {
    fprintf(stdout, "Spec %3d\n", 13);
    int e = 0, push_status, pop_status, *push_value, pop_value;
    int cap = DEF_BOUNDS - 2;
    // push all to left
    while(true) {
        if(test_deque.size >= cap)
            break;
        fprintf(stdout, "\tPush iteration %3d\n", e);
        push_value = (int *) malloc(sizeof(int));
        *push_value = e++;
        left_push(test_deque, push_value, push_status);
        assert(test_deque.size == e);
        assert(push_status == OK);
    }
    // push over half-cap, then make sure hints changed
    assert(test_deque.left_hint.load().nodes != test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 - 1 - DEF_BOUNDS);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2);
    assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == (void *) test_deque.right_hint.load().nodes);
    assert((void *) test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes[0].load().value);
    // pop all from right
    e = 0;
    while(true) {
        if(test_deque.size <= 0)
            break;
        fprintf(stdout, "\tPop iteration %3d\n", e);
        pop_value = *right_pop(test_deque, pop_status);
        assert(pop_value == e++);
        assert(test_deque.size == cap - e);
        assert(pop_status == OK);
    }
    // deque all, then make sure hints changed
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 - 1 - DEF_BOUNDS);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2 - DEF_BOUNDS);
    //assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == RNULL);
    //assert(test_deque.right_hint.load().nodes[0].load().value == LNULL);
    return 0;
}

int spec014(int &stat) {
    fprintf(stdout, "Spec %3d\n", 14);
    int e = 0, push_status, pop_status, *push_value, pop_value;
    int cap = DEF_BOUNDS - 2;
    // push all to right
    while(true) {
        if(test_deque.size >= cap)
            break;
        fprintf(stdout, "\tPush iteration %3d\n", e);
        push_value = (int *) malloc(sizeof(int));
        *push_value = e++;
        right_push(test_deque, push_value, push_status);
        assert(test_deque.size == e);
        assert(push_status == OK);
    }
    // push over half-cap, then make sure hints changed
    assert(test_deque.left_hint.load().nodes != test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 - 1);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2 + DEF_BOUNDS);
    assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == (void *) test_deque.right_hint.load().nodes);
    assert((void *) test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes[0].load().value);
    // pop all from left
    e = 0;
    while(true) {
        if(test_deque.size <= 0)
            break;
        fprintf(stdout, "\tPop iteration %3d\n", e);
        pop_value = *left_pop(test_deque, pop_status);
        assert(pop_value == e++);
        assert(test_deque.size == cap - e);
        assert(pop_status == OK);
    }
    // deque all, then make sure hints changed
    assert(test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 - 1 + DEF_BOUNDS);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2 + DEF_BOUNDS);
    //assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == RNULL);
    //assert(test_deque.right_hint.load().nodes[0].load().value == LNULL);
    return 0;
}

//     15 and 16. check emptiness in the straddling case (left and right)
int spec015(int &stat) {
    fprintf(stdout, "Spec %3d\n", 15);
    int e = 0, push_status, pop_status, *push_value, pop_value;
    int cap = DEF_BOUNDS / 2;
    // push left over half-cap
    while(true) {
        if(test_deque.size >= cap)
            break;
        fprintf(stdout, "\tPush iteration %3d\n", e);
        push_value = (int *) malloc(sizeof(int));
        *push_value = e++;
        left_push(test_deque, push_value, push_status);
        assert(test_deque.size == e);
        assert(push_status == OK);
    }
    assert(test_deque.left_hint.load().nodes != test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 - 19);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2);
    assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == (void *) test_deque.right_hint.load().nodes);
    assert((void *) test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes[0].load().value);
    // pop all from right, pop one from left
    e = 0;
    while(true) {
        if(test_deque.size <= 1)
            break;
        fprintf(stdout, "\tPop iteration %3d\n", e);
        pop_value = *right_pop(test_deque, pop_status);
        assert(pop_value == e++);
        assert(test_deque.size == cap - e);
        assert(pop_status == OK);
    }
    fprintf(stdout, "\tSingle pop other side\n");
    pop_value = *left_pop(test_deque, pop_status);
    assert(pop_value == e++);
    assert(pop_status == OK);
    assert(test_deque.size == 0);
    assert(test_deque.left_hint.load().nodes != test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 - 18);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2 - 15);
    assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == (void *) test_deque.right_hint.load().nodes);
    assert((void *) test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes[0].load().value);
    // left pop status is EMPTY
    left_pop(test_deque, pop_status);
    assert(pop_status == EMPTY);
    // left pop status is EMPTY
    right_pop(test_deque, pop_status);
    assert(pop_status == EMPTY);
}

int spec016(int &stat) {
    fprintf(stdout, "Spec %3d\n", 16);
    int e = 0, push_status, pop_status, *push_value, pop_value;
    int cap = DEF_BOUNDS / 2;
    // push left over half-cap
    while(true) {
        if(test_deque.size >= cap)
            break;
        fprintf(stdout, "\tPush iteration %3d\n", e);
        push_value = (int *) malloc(sizeof(int));
        *push_value = e++;
        right_push(test_deque, push_value, push_status);
        assert(test_deque.size == e);
        assert(push_status == OK);
    }
    assert(test_deque.left_hint.load().nodes != test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 - 1);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2 + 18);
    assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == (void *) test_deque.right_hint.load().nodes);
    assert((void *) test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes[0].load().value);
    // pop all from right, pop one from left
    e = 0;
    while(true) {
        if(test_deque.size <= 1)
            break;
        fprintf(stdout, "\tPop iteration %3d\n", e);
        pop_value = *left_pop(test_deque, pop_status);
        assert(pop_value == e++);
        assert(test_deque.size == cap - e);
        assert(pop_status == OK);
    }
    fprintf(stdout, "\tSingle pop other side\n");
    pop_value = *right_pop(test_deque, pop_status);
    assert(pop_value == e++);
    assert(pop_status == OK);
    assert(test_deque.size == 0);
    assert(test_deque.left_hint.load().nodes != test_deque.right_hint.load().nodes);
    assert(test_deque.left_hint.load().index == DEF_BOUNDS / 2 + 14);
    assert(test_deque.right_hint.load().index == DEF_BOUNDS / 2 + 17);
    assert(test_deque.left_hint.load().nodes[DEF_BOUNDS - 1].load().value == (void *) test_deque.right_hint.load().nodes);
    assert((void *) test_deque.left_hint.load().nodes == test_deque.right_hint.load().nodes[0].load().value);
    // left pop status is EMPTY
    left_pop(test_deque, pop_status);
    assert(pop_status == EMPTY);
    // left pop status is EMPTY
    right_pop(test_deque, pop_status);
    assert(pop_status == EMPTY);
}

int main(int argc, char *argv[]) {
    int (*specs[SPEC_COUNT])(int &stat) = { &spec007, &spec008, &spec009,
        &spec001, &spec002, &spec003, &spec004, &spec005, &spec006, &spec010,
        &spec011, &spec012, &spec013, &spec014, &spec015, &spec016 };
    int i, spec_stat;
    
    for(i = 0; i < SPEC_COUNT; i++) {
        init_deque(test_deque);
        specs[i](spec_stat);
        clear_deque(test_deque);
    }
    
    return 0;
}

