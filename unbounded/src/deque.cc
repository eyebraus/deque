
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

void left_push(deque_t &deque, int *elt, int &stat) {
    deque_hint_t k;
    deque_node_t previous, current;
    
    while(1) {
        k = oracle(deque, LEFT);
        // NOTE: checking for k.node safety is left up to oracle
        previous = k.nodes[(k.index + 1) % DEF_BOUNDS].load();
        current = k.nodes[k.index % DEF_BOUNDS].load();
        
        if(previous.value != LNULL && current.value == LNULL) {
            deque_node_t prev_new, cur_new;
            
            if(k.index % DEF_BOUNDS == 0) {
                atomic_deque_node_t *buffer;
                deque_node_t right_edge, value_node;
                
                // buffer edge: allocate new buffer, attempt to attach
                buffer = (atomic_deque_node_t *) malloc(DEF_BOUNDS * sizeof(atomic_deque_node_t));
                init_buffer(buffer, DEF_BOUNDS);
                // set edge pointer & contents
                set_deque_node(right_edge, (void *) k.nodes, 0);
                set_deque_node(value_node, (void *) elt, 0);
                buffer[DEF_BOUNDS - 1].store(right_edge);
                buffer[DEF_BOUNDS - 2].store(value_node);
                
                // normal swapping case
                copy_deque_node(prev_new, previous);
                set_deque_node(cur_new, (void *) buffer, current.count);
                
                if(k.nodes[(k.index + 1) % DEF_BOUNDS].compare_exchange_strong(previous, prev_new)) {
                    if(k.nodes[k.index % DEF_BOUNDS].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, buffer, k.index - 3);
                        deque.left_hint.compare_exchange_strong(k, new_hint);
                        deque.size++;
                        stat = OK;
                        return;
                    } else {
                        // make sure to free unused buffer!
                        free(buffer);
                    }
                }
            } else {
                // otherwise do the normal swapping case
                copy_deque_node(prev_new, previous);
                set_deque_node(cur_new, (void *) elt, current.count);
                
                if(k.nodes[(k.index + 1) % DEF_BOUNDS].compare_exchange_strong(previous, prev_new)) {
                    if(k.nodes[k.index % DEF_BOUNDS].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, k.nodes, k.index - 1);
                        deque.left_hint.compare_exchange_strong(k, new_hint);
                        deque.size++;
                        stat = OK;
                        return;
                    }
                }
            }
        }
    }
}

int *left_pop(deque_t &deque, int &stat) {
    unsigned long int k;
    deque_node_t current, next;
    
    while(1) {
        k = oracle(deque, LEFT);
        current = deque.nodes[(k.index + 1) % DEF_BOUNDS].load();
        next = deque.nodes[k.index % DEF_BOUNDS].load();
        
        if(current.value != LNULL && next.value == LNULL) {
            deque_node_t cur_new, next_new;
            
            if(k.index % DEF_BOUNDS == DEF_BOUNDS - 1) {
                // edge buffer: attempt to reclaim
                // HAZARD POINTERS AND SHIT!!!
            } else {
                // otherwise, do the normal swap op
                set_deque_node(cur_new, LNULL, current.count);
                set_deque_node(next_new, LNULL, next.count);
                
                if(k.nodes[k.index % DEF_BOUNDS].compare_exchange_strong(next, next_new)) {
                    if(k.nodes[(k.index + 1) % DEF_BOUNDS].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, k.nodes, k.index + 1);
                        deque.left_hint.compare_exchange_strong(k, new_hint);
                        deque.size--;
                        stat = OK;
                        return current.value;
                    }
                }
            }
        }
    }
}

void right_push(deque_t &deque, int *elt, int &stat) {
    unsigned long int k;
    deque_node_t previous, current;
    
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

            deque_node_t prev_new, cur_new;
            copy_deque_node(prev_new, previous);
            set_deque_node(cur_new, (void *) elt, current.count);
            
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

int *right_pop(deque_t &deque, int &stat) {
    unsigned long int k;
    deque_node_t current, next;
    
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
            
            deque_node_t cur_new, next_new;
            set_deque_node(cur_new, RNULL, current.count);
            set_deque_node(next_new, RNULL, next.count);
            
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

deque_hint_t oracle(deque_t &deque, oracle_end deque_end) {
    unsigned long int i, k;
    deque_node_t current, previous;
    
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
                    return k;
            }
            return deque.right_hint.load();
        } else {
            return deque.right_hint.load();
        }
    }
}

void init_deque_node(atomic_deque_node_t &node, void *init_null) {
    deque_node_t blank_node;
    blank_node.value = init_null;
    blank_node.count = 0;
    node.store(blank_node);
}

void init_deque(deque_t &deque) {
    atomic_deque_node_t *buffer;
    deque_hint_t left_hint, right_hint;
    
    buffer = (atomic_deque_node_t *) malloc(DEF_BOUNDS * sizeof(atomic_deque_node_t));
    init_buffer(buffer, DEF_BOUNDS);
    
    deque.size = 0;
    set_deque_hint(left_hint, buffer, DEF_BOUNDS / 2 - 1);
    set_deque_hint(right_hint, buffer, DEF_BOUNDS / 2);
    deque.left_hint.store(left_hint);
    deque.right_hint.store(right_hint);
}

void init_buffer(atomic_deque_node_t *buffer, int size) {
    int i;
    
    for(i = 0; i < size; i++) {
        if(i < size / 2)
            init_deque_node(buffer[i], LNULL);
        else 
            init_deque_node(buffer[i], RNULL);
    }
}

void clear_deque_node(atomic_deque_node_t &node) {
    deque_node_t blank_node, old_node;
    blank_node.value = NULL;
    blank_node.count = 0;
    old_node = node.exchange(blank_node);
    if(!is_null(old_node.value))
        free(old_node.value);
}

void clear_deque(deque_t &deque) {
    bool left_check;
    atomic_deque_node_t *buffer, *bufsave, *left_next, *right_next;
    deque_hint_t left_hint, right_hint;
    
    buffer = deque.left_hint.load().nodes;
    left_next = (atomic_deque_node_t *) buffer[0].load().value;
    right_next = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
    clear_buffer(buffer, DEF_BOUNDS);
    
    left_check = !is_null(left_next);
    bufsave = buffer;
    while(!is_null(left_next) && !is_null(right_next)) {
        if(left_check) {
            buffer = left_next;
            left_next = (atomic_deque_node_t *) buffer[0].load().value;
            clear_buffer(buffer, DEF_BOUNDS);
            // free!
            free(buffer);
        } else {
            buffer = right_next;
            right_next = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
            clear_buffer(buffer, DEF_BOUNDS);
            // free!
            free(buffer);
        }
    }

    deque.size = 0;
    buffer = bufsave;
    set_deque_hint(left_hint, buffer, DEF_BOUNDS / 2 - 1);
    set_deque_hint(right_hint, buffer, DEF_BOUNDS / 2);
    deque.left_hint.store(left_hint);
    deque.right_hint.store(right_hint);
}

void clear_buffer(atomic_deque_node_t *buffer, int size) {
    int i;
    
    for(i = 0; i < size; i++) {
        clear_deque_node(buffer[i]);
    }
}

void set_deque_node(deque_node_t &node, void *value, unsigned int last_count) {
    node.value = value;
    node.count = last_count + 1;
}

void copy_deque_node(deque_node_t &new_node, deque_node_t &old_node) {
    new_node.value = old_node.value;
    new_node.count = old_node.count + 1;
}

