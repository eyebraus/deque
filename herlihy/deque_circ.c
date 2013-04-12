
#include <stdio.h>
#include <stdlib.h>
#include "atomic_ops.h"
#include "deque.h"

int left_push(bounded_deque_t *deque, int elt) {
    // fill in later
}

double *left_pop(bounded_deque_t *deque) {
    // fill in later

}

oracle_result_t *left_checked_oracle(bounded_deque_t *deque) {
    // fill in later

}

int right_push(bounded_deque_t *deque, int elt) {
    unsigned long long int k;
    bounded_deque_node_t previous, current, next;
    oracle_result_t *result = NULL;
    
    while(1) {
        result = right_checked_oracle(deque);
        k = result->k;
        previous = result->left;
        current = result->right;
        next = deque->nodes[k + 1];
        free(result);
       
        // RN -> v 
        if(next.value == RNULL) {
            bounded_deque_node_t prev_new, cur_new;
            copy_bounded_deque_node(&prev_new, &previous);
            set_bounded_deque_node(&cur_new, &elt, current.count);
            if(CAS_NODE(&deque->nodes[k - 1], previous, prev_new)) {
                if(CAS_NODE(&deque->nodes[k], current, cur_new)) {
                    deque->size++;
                    return OK;
                }
            }
        }
        
        // LN -> DN, try again
        if(next.value == LNULL) {
            bounded_deque_node_t cur_new, next_new;
            set_bounded_deque_node(&cur_new, RNULL, current.count);
            if(CAS_NODE(&deque->nodes[k], current, cur_new)) {
                set_bounded_deque_node(&next_new, DNULL, next.count);
                CAS_NODE(&deque->nodes[k + 1], next, next_new);
            }
        }
        
        // DN -> RN, try again
        if(next.value == DNULL) {
            bounded_deque_node_t nextnext = deque->nodes[k + 2];
            if(!IS_NULL(nextnext.value)) {
                if(ATOMIC_EQL(&deque->nodes[k - 1], previous))
                    if(ATOMIC_EQL(&deque->nodes[k], current))
                        return FULL;
            } else if(nextnext.value == LNULL) {
                bounded_deque_node_t next_new, nextnext_new;
                copy_bounded_deque_node(&nextnext_new, &nextnext);
                if(CAS_NODE(&deque->nodes[k + 2], nextnext, nextnext_new)) {
                    set_bounded_deque_node(&next_new, RNULL, next.count);
                    CAS_NODE(&deque->nodes[k + 1], next, next_new);
                }
            }
        }
    }
}

int right_pop(bounded_deque_t *deque, int *stat) {
    unsigned long long int k;
    bounded_deque_node_t current, next;
    oracle_result_t *result = NULL;
    
    while(1) {
        result = right_checked_oracle(deque);
        k = result->k;
        current = result->left;
        next = result->right;
        free(result);
        
        // empty check
        if((current.value == LNULL || current.value == DNULL) && ATOMIC_EQL(&deque->nodes[k - 1], current)) {
            *stat = EMPTY;
            return *current.value;
        } else {
            bounded_deque_node_t cur_new, next_new;
            set_bounded_deque_node(&current, (int *) RNULL, current.count);
            set_bounded_deque_node(&next, (int *) RNULL, next.count);
            if(CAS_NODE(&deque->nodes[k], next, next_new)) {
                if(CAS_NODE(&deque->nodes[k - 1], current, cur_new)) {
                    *stat = OK;
                    return *current.value;
                }
            }
        }
    }
}

oracle_result_t *right_checked_oracle(bounded_deque_t *deque) {
    unsigned long long int k;
    bounded_deque_node_t left, right;
    oracle_result_t *result = (oracle_result_t *) malloc(sizeof(oracle_result_t));
    
    while(1) {
        k = oracle(RIGHT); 
        left = deque->nodes[k - 1];
        right = deque->nodes[k];
        if(right.value == RNULL && left.value != RNULL) {
            result->k = k;
            result->left = left;
            result->right = right;
            return result;
        }
        if(right.value == DNULL && left.value != RNULL && left.value != DNULL) {
            bounded_deque_node_t left_new, right_new;
            copy_bounded_deque_node(&left_new, &left);
            set_bounded_deque_node(&right_new, RNULL, right.count);
            if(CAS_NODE(&deque->nodes[k - 1], left, left_new)) {
                if(CAS_NODE(&deque->nodes[k], right, right_new)) {
                    result->k = k;
                    result->left = left;
                    result->right = right;
                    return result;
                }
            }
        }
    }
}

unsigned long long int oracle(oracle_ends end) {
    // fill in later
    
}

void init_bounded_deque_node(bounded_deque_node_t *node) {
    node->value = NULL;
    node->count = 0;
}

void init_bounded_deque(bounded_deque_t *deque) {
    int i;
    
    deque->size = 0;
    for(i = 0; i < DEF_BOUNDS; i++) {
        init_bounded_deque_node(&deque->nodes[i]);
        if(i == 0)
            deque->nodes[i].value = DNULL;
        else if(i < DEF_BOUNDS / 2)
            deque->nodes[i].value = LNULL;
        else 
            deque->nodes[i].value = RNULL;
    }
}

void init_oracle_result(oracle_result_t *result) {
    result->k = 0;
    init_bounded_deque_node(&result->left);
    init_bounded_deque_node(&result->right);
}

void clear_bounded_deque_node(bounded_deque_node_t *node) {
    if(!IS_NULL(node->value))
        free(node->value);
    node->value = NULL;
    // do not change the count!!!
}

void clear_bounded_deque(bounded_deque_t *deque) {
    int i;
    
    for(i = 0; i < DEF_BOUNDS; i++) {
        clear_bounded_deque_node(&deque->nodes[i]);
        // clear counts here
        deque->nodes[i].count = 0;
    }
    deque->size = 0;
}

void clear_oracle_result(oracle_result_t *result) {
    result->k = 0;
    clear_bounded_deque_node(&result->left);
    clear_bounded_deque_node(&result->right);
}

void set_bounded_deque_node(bounded_deque_node_t *node, int *value, unsigned int last_count) {
    if(IS_NULL(value))
        node->value = value;
    else {
        node->value = (int *) malloc(sizeof(int));
        *node->value = *value;
    }
    node->count = last_count + 1;
}

void copy_bounded_deque_node(bounded_deque_node_t *old_node, bounded_deque_node_t *new_node) {
    int *value = new_node->value;

    if(IS_NULL(value))
        new_node->value = value;
    else {
        new_node->value = (int *) malloc(sizeof(int));
        *new_node->value = *value;
    }
    new_node->count = old_node->count + 1;
}

int main(int argc, char *argv[]) {
    printf("LNULL: %d, RNULL: %d, DNULL: %d\n", LNULL, RNULL, DNULL);
    return 0;
}
