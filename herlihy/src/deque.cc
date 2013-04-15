
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include "deque_circ.h"

/*
 * TODOs and stuff
 * ---------------
 *  - do the left/right hints really need to be atomic?
 *  - what needs to be atomic?
 *  - is my EQL_NODE macro legit?
 */

void left_push(bounded_deque_t *deque, int elt, int *stat) {
    atomic_ulong k;
    bounded_deque_node_t previous, current;
    
    while(1) {
        k = deque->left_hint;
        previous = deque->nodes[k + 1];
        current = deque->nodes[k];
        
        if(previous.value != LNULL && current.value == LNULL) {
            if(k <= 0) {
                *stat = FULL;
                return;
            }

            bounded_deque_node_t prev_new, cur_new;
            copy_bounded_deque_node(&prev_new, &previous);
            set_bounded_deque_node(&cur_new, &elt, current.count);
            
            if(CAS_NODE(&deque->nodes[k + 1], &previous, prev_new)) {
                if(CAS_NODE(&deque->nodes[k], &current, cur_new)) {
                    // update loc hint
                    atomic_fetch_add(&deque->left_hint, -1);
                    *stat = OK;
                    return;
                }
            }
        }
    }
}

int left_pop(bounded_deque_t *deque, int *stat) {
    atomic_ulong k;
    bounded_deque_node_t current, next;
    int ret_val;
    
    while(1) {
        k = deque->left_hint;
        current = deque->nodes[k + 1];
        next = deque->nodes[k];
        
        if(current.value != LNULL && next.value == LNULL) {
            if(current.value == RNULL && EQL_NODE(deque->nodes[k + 1], current)) {
                *stat = EMPTY;
                return *current.value;
            }
            
            bounded_deque_node_t cur_new, next_new;
            set_bounded_deque_node(&cur_new, LNULL, current.count);
            set_bounded_deque_node(&next_new, LNULL, next.count);
            
            if(CAS_NODE(&deque->nodes[k], next, next_new)) {
                if(CAS_NODE(&deque->nodes[k + 1], current, cur_new)) {
                    // update loc hint
                    atomic_fetch_add(&deque->left_hint, 1);
                    // FREE OLD INT
                    ret_val = *current.value;
                    if(!IS_NULL(current.value))
                        free(current.value);
                    *stat = OK;
                    return ret_val;
                }
            }
        }
    }
}

void right_push(bounded_deque_t *deque, int elt, int *stat) {
    unsigned long long int k;
    bounded_deque_node_t previous, current;
    
    while(1) {
        k = deque->right_hint;
        previous = deque->nodes[k - 1];
        current = deque->nodes[k];
        
        if(previous.value != RNULL && current.value == RNULL) {
            if(k >= DEF_BOUNDS) {
                *stat = FULL;
                return;
            }

            bounded_deque_node_t prev_new, cur_new;
            copy_bounded_deque_node(&prev_new, &previous);
            set_bounded_deque_node(&cur_new, &elt, current.count);
            
            if(CAS_NODE(&deque->nodes[k - 1], previous, prev_new)) {
                if(CAS_NODE(&deque->nodes[k], current, cur_new)) {
                    // update loc hint
                    atomic_fetch_add(&deque->right_hint, 1);
                    *stat = OK;
                    return;
                }
            }
        }
    }
}

int right_pop(bounded_deque_t *deque, int *stat) {
    atomic_ulong k;
    bounded_deque_node_t current, next;
    int ret_val;
    
    while(1) {
        k = deque->right_hint;
        current = deque->nodes[k - 1];
        next = deque->nodes[k];
        
        if(current.value != RNULL && next.value == RNULL) {
            if(current.value == LNULL && EQL_NODE(deque->nodes[k - 1], current)) {
                *stat = EMPTY;
                return *current.value;
            }
            
            bounded_deque_node_t cur_new, next_new;
            set_bounded_deque_node(&cur_new, RNULL, current.count);
            set_bounded_deque_node(&next_new, RNULL, next.count);
            
            if(CAS_NODE(&deque->nodes[k], next, next_new)) {
                if(CAS_NODE(&deque->nodes[k - 1], current, cur_new)) {
                    // update loc hint
                    atomic_fetch_add(&deque->right_hint, -1);
                    // FREE OLD INT
                    ret_val = *current.value;
                    if(!IS_NULL(current.value))
                        free(current.value);
                    *stat = OK;
                    return ret_val;
                }
            }
        }
    }
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
        if(i < DEF_BOUNDS / 2)
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

}
