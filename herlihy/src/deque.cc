
#include <atomic>
#include <stdio.h>
#include <stdlib.h>
#include "deque.h"

using namespace std;

/*
 * TODOs and stuff
 * ---------------
 *  - do the left/right hints really need to be atomic?
 *  - what needs to be atomic?
 *  - is my EQL_NODE macro legit?
 */

void left_push(bounded_deque_t &deque, int elt, int &stat) {
    unsigned long int k;
    bounded_deque_node_t previous, current;
    
    while(1) {
        k = deque.left_hint.load();
        previous = deque.nodes[k + 1].load(memory_order_acquire);
        current = deque.nodes[k].load(memory_order_acquire);
        
        if(previous.value != LNULL && current.value == LNULL) {
            if(k <= 0) {
                stat = FULL;
                return;
            }

            bounded_deque_node_t prev_new, cur_new;
            copy_bounded_deque_node(prev_new, previous);
            set_bounded_deque_node(cur_new, &elt, current.count);
            
            if(deque.nodes[k + 1].compare_exchange_strong(previous, prev_new)) {
                if(deque.nodes[k].compare_exchange_strong(current, cur_new)) {
                    // update loc hint
                    deque.left_hint--;
                    deque.size++;
                    stat = OK;
                    return;
                }
            }
        }
    }
}

int left_pop(bounded_deque_t &deque, int &stat) {
    unsigned long int k;
    bounded_deque_node_t current, next;
    int ret_val;
    
    while(1) {
        k = deque.left_hint.load();
        current = deque.nodes[k + 1].load(memory_order_acquire);
        next = deque.nodes[k].load(memory_order_acquire);
        
        if(current.value != LNULL && next.value == LNULL) {
            if(current.value == RNULL && compare_node(deque.nodes[k + 1], current)) {
                stat = EMPTY;
                return 0;
            }
            
            bounded_deque_node_t cur_new, next_new;
            set_bounded_deque_node(cur_new, LNULL, current.count);
            set_bounded_deque_node(next_new, LNULL, next.count);
            
            if(deque.nodes[k].compare_exchange_strong(next, next_new)) {
                if(deque.nodes[k + 1].compare_exchange_strong(current, cur_new)) {
                    // update loc hint
                    deque.left_hint++;
                    deque.size--;
                    // FREE OLD INT
                    ret_val = *current.value;
                    if(!is_null(current.value))
                        free(current.value);
                    stat = OK;
                    return ret_val;
                }
            }
        }
    }
}

void right_push(bounded_deque_t &deque, int elt, int &stat) {
    unsigned long int k;
    bounded_deque_node_t previous, current;
    
    while(1) {
        k = deque.right_hint.load();
        previous = deque.nodes[k - 1].load(memory_order_acquire);
        current = deque.nodes[k].load(memory_order_acquire);
        
        if(previous.value != RNULL && current.value == RNULL) {
            if(k >= DEF_BOUNDS - 1) {
                stat = FULL;
                return;
            }

            bounded_deque_node_t prev_new, cur_new;
            copy_bounded_deque_node(prev_new, previous);
            set_bounded_deque_node(cur_new, &elt, current.count);
            
            if(deque.nodes[k - 1].compare_exchange_strong(previous, prev_new)) {
                if(deque.nodes[k].compare_exchange_strong(current, cur_new)) {
                    // update loc hint
                    deque.right_hint++;
                    deque.size++;
                    stat = OK;
                    return;
                }
            }
        }
    }
}

int right_pop(bounded_deque_t &deque, int &stat) {
    unsigned long int k;
    bounded_deque_node_t current, next;
    int ret_val;
    
    while(1) {
        k = deque.right_hint.load();
        current = deque.nodes[k - 1].load(memory_order_acquire);
        next = deque.nodes[k].load(memory_order_acquire);
        
        if(current.value != RNULL && next.value == RNULL) {
            if(current.value == LNULL && compare_node(deque.nodes[k - 1], current)) {
                stat = EMPTY;
                return 0;
            }
            
            bounded_deque_node_t cur_new, next_new;
            set_bounded_deque_node(cur_new, RNULL, current.count);
            set_bounded_deque_node(next_new, RNULL, next.count);
            
            if(deque.nodes[k].compare_exchange_strong(next, next_new)) {
                if(deque.nodes[k - 1].compare_exchange_strong(current, cur_new)) {
                    // update loc hint
                    deque.right_hint--;
                    deque.size--;
                    // FREE OLD INT
                    ret_val = *current.value;
                    if(!is_null(current.value))
                        free(current.value);
                    stat = OK;
                    return ret_val;
                }
            }
        }
    }
}

void init_bounded_deque_node(atomic_deque_node_t &node, int *init_null) {
    bounded_deque_node_t blank_node;
    blank_node.value = init_null;
    blank_node.count = 0;
    node.store(blank_node);
}

void init_bounded_deque(bounded_deque_t &deque) {
    int i;
    
    for(i = 0; i < DEF_BOUNDS; i++) {
        if(i < DEF_BOUNDS / 2)
            init_bounded_deque_node(deque.nodes[i], LNULL);
        else 
            init_bounded_deque_node(deque.nodes[i], RNULL);
    }
    
    deque.size = 0;
    deque.left_hint = DEF_BOUNDS / 2 - 1;
    deque.right_hint = DEF_BOUNDS / 2;
}

void clear_bounded_deque_node(atomic_deque_node_t &node) {
    int *val_save;
    bounded_deque_node_t blank_node, old_node;
    blank_node.value = NULL;
    blank_node.count = 0;
    old_node = node.exchange(blank_node);
    if(!is_null(old_node.value))
        free(old_node.value);
}

void clear_bounded_deque(bounded_deque_t &deque) {
    int i;
    
    for(i = 0; i < DEF_BOUNDS; i++) {
        if(i < DEF_BOUNDS / 2)
            init_bounded_deque_node(deque.nodes[i], LNULL);
        else 
            init_bounded_deque_node(deque.nodes[i], RNULL);
    }
    
    deque.size = 0;
    deque.left_hint = DEF_BOUNDS / 2 - 1;
    deque.right_hint = DEF_BOUNDS / 2;
}

void set_bounded_deque_node(bounded_deque_node_t &node, int *value, unsigned int last_count) {
    if(is_null(value))
        node.value = value;
    else {
        node.value = (int *) malloc(sizeof(int));
        *node.value = *value;
    }
    node.count = last_count + 1;
}

void copy_bounded_deque_node(bounded_deque_node_t &new_node, bounded_deque_node_t &old_node) {
    int *value = old_node.value;

    if(is_null(value))
        new_node.value = value;
    else {
        new_node.value = (int *) malloc(sizeof(int));
        *new_node.value = *value;
    }
    new_node.count = old_node.count + 1;
}

