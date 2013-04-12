
#include <stdio.h>
#include <stdlib.h>
#include "atomic_ops.h"
#include "deque_simple.h"

void left_push(bounded_deque_t *deque, int elt, int *stat) {
    unsigned long long int k;
    bounded_deque_node_t previous, current;
    
    while(1) {
        k = oracle(LEFT);
        previous = deque->nodes[k + 1];
        current = deque->nodes[k];
        
        if(previous.value != LNULL && current.value == LNULL) {
            if(k <= -1) {
                *stat = FULL;
                return;
            }

            bounded_deque_node_t prev_new, cur_new;
            copy_bounded_deque_node(&prev_new, &previous);
            set_bounded_deque_node(&cur_new, &elt, current.count);
            
            if(CAS_NODE(&deque->nodes[k + 1], previous, prev_new)) {
                if(CAS_NODE(&deque->nodes[k], current, cur_new)) {
                    deque->size++;
                    *stat = OK;
                    return;
                }
            }
        }
    }
}

int *left_pop(bounded_deque_t *deque, int *stat) {
    unsigned long long int k;
    bounded_deque_node_t current, next;
    
    while(1) {
        k = oracle(LEFT);
        current = deque->nodes[k + 1];
        next = deque->nodes[k];
        
        if(current.value != LNULL && next.value == LNULL) {
            if(current.value == RNULL && ATOMIC_EQL(&deque->nodes[k + 1], current)) {
                *stat = EMPTY;
                return *current.value;
            }
            
            bounded_deque_node_t cur_new, next_new;
            set_bounded_deque_node(&cur_new, LNULL, current.count);
            set_bounded_deque_node(&next_new, LNULL, next.count);
            
            if(CAS_NODE(&deque->nodes[k], next, next_new)) {
                if(CAS_NODE(&deque->nodes[k + 1], current, cur_new)) {
                    deque->size--;
                    *stat = OK;
                    return *current.value;
                }
            }
        }
    }
}

void right_push(bounded_deque_t *deque, int elt, int *stat) {
    unsigned long long int k;
    bounded_deque_node_t previous, current;
    
    while(1) {
        k = oracle(RIGHT);
        previous = deque->nodes[k - 1];
        current = deque->nodes[k];
        
        if(previous.value != RNULL && current.value == RNULL) {
            if(k >= DEF_BOUNDS + 1) {
                *stat = FULL;
                return;
            }

            bounded_deque_node_t prev_new, cur_new;
            copy_bounded_deque_node(&prev_new, &previous);
            set_bounded_deque_node(&cur_new, &elt, current.count);
            
            if(CAS_NODE(&deque->nodes[k - 1], previous, prev_new)) {
                if(CAS_NODE(&deque->nodes[k], current, cur_new)) {
                    deque->size++;
                    *stat = OK;
                    return;
                }
            }
        }
    }
}

int right_pop(bounded_deque_t *deque, int *stat) {
    unsigned long long int k;
    bounded_deque_node_t current, next;
    
    while(1) {
        k = oracle(RIGHT);
        current = deque->nodes[k - 1];
        next = deque->nodes[k];
        
        if(current.value != RNULL && next.value == RNULL) {
            if(current.value == LNULL && ATOMIC_EQL(&deque->nodes[k - 1], current)) {
                *stat = EMPTY;
                return *current.value;
            }
            
            bounded_deque_node_t cur_new, next_new;
            set_bounded_deque_node(&cur_new, RNULL, current.count);
            set_bounded_deque_node(&next_new, RNULL, next.count);
            
            if(CAS_NODE(&deque->nodes[k], next, next_new)) {
                if(CAS_NODE(&deque->nodes[k - 1], current, cur_new)) {
                    deque->size--;
                    *stat = OK;
                    return *current.value;
                }
            }
        }
    }
}

unsigned long long int oracle(oracle_ends end) {
    if(end == LEFT) {

    } else if(end == RIGHT) {

    } else {
        
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
