
#include <atomic>
#include <stdio.h>
#include <stdlib.h>
#include "deque.h"

using namespace std;

/*
 * TODOs and stuff
 * ---------------
 *     - actually write oracle function. the hints are, in fact, just hints
 */

void left_push(bounded_deque_t &deque, int *elt, int &stat) {
    unsigned long int k;
    bounded_deque_node_t previous, current;
    
    while(1) {
        k = oracle(deque, LEFT);
        //k = deque.left_hint.load();
        previous = deque.nodes[k + 1].load();
        current = deque.nodes[k].load();
        
        if(previous.value != LNULL && current.value == LNULL) {
            if(k <= 0) {
                stat = FULL;
                return;
            }

            bounded_deque_node_t prev_new, cur_new;
            copy_bounded_deque_node(prev_new, previous);
            set_bounded_deque_node(cur_new, elt, current.count);
            
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

int *left_pop(bounded_deque_t &deque, int &stat) {
    unsigned long int k;
    bounded_deque_node_t current, next;
    
    while(1) {
        k = oracle(deque, LEFT);
        //k = deque.left_hint.load();
        current = deque.nodes[k + 1].load();
        next = deque.nodes[k].load();
        
        if(current.value != LNULL && next.value == LNULL) {
            if(current.value == RNULL && compare_node(deque.nodes[k + 1], current)) {
                stat = EMPTY;
                return NULL;
            }
            
            bounded_deque_node_t cur_new, next_new;
            set_bounded_deque_node(cur_new, LNULL, current.count);
            set_bounded_deque_node(next_new, LNULL, next.count);
            
            if(deque.nodes[k].compare_exchange_strong(next, next_new)) {
                if(deque.nodes[k + 1].compare_exchange_strong(current, cur_new)) {
                    // update loc hint
                    deque.left_hint++;
                    deque.size--;
                    stat = OK;
                    return current.value;
                }
            }
        }
    }
}

void right_push(bounded_deque_t &deque, int *elt, int &stat) {
    unsigned long int k;
    bounded_deque_node_t previous, current;
    
    while(1) {
        k = oracle(deque, RIGHT);
        //k = deque.right_hint.load();
        previous = deque.nodes[k - 1].load();
        current = deque.nodes[k].load();
        
        if(previous.value != RNULL && current.value == RNULL) {
            if(k >= DEF_BOUNDS - 1) {
                stat = FULL;
                return;
            }

            bounded_deque_node_t prev_new, cur_new;
            copy_bounded_deque_node(prev_new, previous);
            set_bounded_deque_node(cur_new, elt, current.count);
            
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

int *right_pop(bounded_deque_t &deque, int &stat) {
    unsigned long int k;
    bounded_deque_node_t current, next;
    
    while(1) {
        k = oracle(deque, RIGHT);
        //k = deque.right_hint.load();
        current = deque.nodes[k - 1].load();
        next = deque.nodes[k].load();
        
        if(current.value != RNULL && next.value == RNULL) {
            if(current.value == LNULL && compare_node(deque.nodes[k - 1], current)) {
                stat = EMPTY;
                return NULL;
            }
            
            bounded_deque_node_t cur_new, next_new;
            set_bounded_deque_node(cur_new, RNULL, current.count);
            set_bounded_deque_node(next_new, RNULL, next.count);
            
            if(deque.nodes[k].compare_exchange_strong(next, next_new)) {
                if(deque.nodes[k - 1].compare_exchange_strong(current, cur_new)) {
                    // update loc hint
                    deque.right_hint--;
                    deque.size--;
                    stat = OK;
                    return current.value;
                }
            }
        }
    }
}

unsigned long int oracle(bounded_deque_t &deque, oracle_end deque_end) {
    unsigned long int i, k;
    bounded_deque_node_t current, previous, next;
    
    if(deque_end == LEFT) {
        k = deque.left_hint.load();
        current = deque.nodes[k].load();
        previous = deque.nodes[k + 1].load();
        
        if(current.value == LNULL && previous.value != LNULL) {
            return k;
        } else if(current.value == LNULL && previous.value == LNULL) {
            // go right
            for(i = k; i < DEF_BOUNDS - 1; i++) {
                current = deque.nodes[i].load();
                previous = deque.nodes[i + 1].load();
                if(current.value == LNULL && previous.value != LNULL)
                    return i;
            }
            return deque.left_hint.load();
        } else {
            return deque.left_hint.load();
        }
    } else if(deque_end == RIGHT) {
        k = deque.right_hint.load();
        current = deque.nodes[k].load();
        previous = deque.nodes[k - 1].load();
        
        if(current.value == RNULL && previous.value != RNULL) {
            return k;
        } else if(current.value == RNULL && previous.value == RNULL) {
            // go left
            for(i = k; i > 0; i--) {
                current = deque.nodes[i].load();
                previous = deque.nodes[i - 1].load();
                if(current.value == RNULL && previous.value != RNULL)
                    return i;
            }
            return deque.right_hint.load();
        } else {
            return deque.right_hint.load();
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
    bounded_deque_node_t blank_node, old_node;
    blank_node.value = NULL;
    blank_node.count = 0;
    old_node = node.exchange(blank_node);
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
    node.value = value;
    node.count = last_count + 1;
}

void copy_bounded_deque_node(bounded_deque_node_t &new_node, bounded_deque_node_t &old_node) {
    new_node.value = old_node.value;
    new_node.count = old_node.count + 1;
}

