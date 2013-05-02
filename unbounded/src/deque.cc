
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
        previous = k.nodes[mod(k.index + 1, DEF_BOUNDS)].load();
        current = k.nodes[mod(k.index, DEF_BOUNDS)].load();
        
        // TODO: FIX THIS!!!! you should push whenever current is at 0
        if(previous.value != LNULL && current.value == LNULL) {
            deque_node_t prev_new, cur_new;
            
            if(mod(k.index, DEF_BOUNDS) == 0) {
                atomic_deque_node_t *buffer;
                deque_node_t right_edge, value_node;
                
                // buffer edge: allocate new buffer, attempt to attach
                buffer = (atomic_deque_node_t *) malloc(DEF_BOUNDS * sizeof(atomic_deque_node_t));
                init_buffer(buffer, DEF_BOUNDS, LBUF);
                // set edge pointer & contents
                set_deque_node(right_edge, (void *) k.nodes, 0);
                set_deque_node(value_node, (void *) elt, 0);
                buffer[DEF_BOUNDS - 1].store(right_edge);
                buffer[DEF_BOUNDS - 2].store(value_node);
                
                // normal swapping case
                copy_deque_node(prev_new, previous);
                set_deque_node(cur_new, (void *) buffer, current.count);
                
                if(k.nodes[mod(k.index + 1, DEF_BOUNDS)].compare_exchange_strong(previous, prev_new)) {
                    if(k.nodes[mod(k.index, DEF_BOUNDS)].compare_exchange_strong(current, cur_new)) {
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
                
                if(k.nodes[mod(k.index + 1, DEF_BOUNDS)].compare_exchange_strong(previous, prev_new)) {
                    if(k.nodes[mod(k.index, DEF_BOUNDS)].compare_exchange_strong(current, cur_new)) {
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
    deque_hint_t k;
    deque_node_t current, next;
    
    while(1) {
        k = oracle(deque, LEFT);
        current = k.nodes[mod(k.index + 1, DEF_BOUNDS)].load();
        next = k.nodes[mod(k.index, DEF_BOUNDS)].load();
        
        if(current.value != LNULL && (next.value == LNULL || mod(k.index, DEF_BOUNDS) == 0)) {
            deque_node_t cur_new, next_new;
            
            if(current.value != RNULL && mod(k.index + 1, DEF_BOUNDS) == DEF_BOUNDS - 1) {
                // check for emptiness, or try to clean up hint
                atomic_deque_node_t *next_right;
                deque_node_t right_peek, right_peek_new;
                
                next_right = (atomic_deque_node_t *) k.nodes[mod(k.index + 1, DEF_BOUNDS)].load().value;
                right_peek = next_right[mod(k.index + 3, DEF_BOUNDS)].load();
                // empty if peek val is null (straddling case)
                if(right_peek.value == RNULL && compare_val(next_right[mod(k.index + 3, DEF_BOUNDS)], right_peek)) {
                    stat = EMPTY;
                    return NULL;
                }
                
                copy_deque_node(right_peek_new, right_peek);
                copy_deque_node(next_new, next);
                
                if(k.nodes[mod(k.index, DEF_BOUNDS)].compare_exchange_strong(next, next_new)) {
                    if(next_right[mod(k.index + 3, DEF_BOUNDS)].compare_exchange_strong(right_peek, right_peek_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, next_right, k.index + 2);
                        deque.left_hint.compare_exchange_strong(k, new_hint);
                    }
                }
            } else {
                if(current.value == RNULL && compare_val(k.nodes[mod(k.index + 1, DEF_BOUNDS)], current)) {
                    stat = EMPTY;
                    return NULL;
                }
                
                // otherwise, do the normal swap op
                set_deque_node(cur_new, LNULL, current.count);
                set_deque_node(next_new, LNULL, next.count);
                
                if(k.nodes[mod(k.index, DEF_BOUNDS)].compare_exchange_strong(next, next_new)) {
                    if(k.nodes[mod(k.index + 1, DEF_BOUNDS)].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, k.nodes, k.index + 1);
                        deque.left_hint.compare_exchange_strong(k, new_hint);
                        deque.size--;
                        stat = OK;
                        return (int *) current.value;
                    }
                }
            }
        }
    }
}

void right_push(deque_t &deque, int *elt, int &stat) {
    deque_hint_t k;
    deque_node_t previous, current;
    
    while(1) {
        k = oracle(deque, RIGHT);
        // NOTE: checking for k.node safety is left up to oracle
        previous = k.nodes[mod(k.index - 1, DEF_BOUNDS)].load();
        current = k.nodes[mod(k.index, DEF_BOUNDS)].load();
        
        // TODO: FIX THIS!!!! you should push whenever current is at SIZE - 1
        if(previous.value != RNULL && current.value == RNULL) {
            deque_node_t prev_new, cur_new;
            
            if(mod(k.index, DEF_BOUNDS) == DEF_BOUNDS - 1) {
                atomic_deque_node_t *buffer;
                deque_node_t left_edge, value_node;
                
                // buffer edge: allocate new buffer, attempt to attach
                buffer = (atomic_deque_node_t *) malloc(DEF_BOUNDS * sizeof(atomic_deque_node_t));
                init_buffer(buffer, DEF_BOUNDS, RBUF);
                // set edge pointer & contents
                set_deque_node(left_edge, (void *) k.nodes, 0);
                set_deque_node(value_node, (void *) elt, 0);
                buffer[0].store(left_edge);
                buffer[1].store(value_node);
                
                // normal swapping case
                copy_deque_node(prev_new, previous);
                set_deque_node(cur_new, (void *) buffer, current.count);
                
                if(k.nodes[mod(k.index - 1, DEF_BOUNDS)].compare_exchange_strong(previous, prev_new)) {
                    if(k.nodes[mod(k.index, DEF_BOUNDS)].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, buffer, k.index + 3);
                        deque.right_hint.compare_exchange_strong(k, new_hint);
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
                
                if(k.nodes[mod(k.index - 1, DEF_BOUNDS)].compare_exchange_strong(previous, prev_new)) {
                    if(k.nodes[mod(k.index, DEF_BOUNDS)].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, k.nodes, k.index + 1);
                        deque.right_hint.compare_exchange_strong(k, new_hint);
                        deque.size++;
                        stat = OK;
                        return;
                    }
                }
            }
        }
    }
}

int *right_pop(deque_t &deque, int &stat) {
    deque_hint_t k;
    deque_node_t current, next;
    
    while(1) {
        k = oracle(deque, RIGHT);
        current = k.nodes[mod(k.index - 1, DEF_BOUNDS)].load();
        next = k.nodes[mod(k.index, DEF_BOUNDS)].load();
        
        if(current.value != RNULL && (next.value == RNULL || mod(k.index, DEF_BOUNDS) == DEF_BOUNDS - 1)) {
            deque_node_t cur_new, next_new;
            
            if(current.value != LNULL && mod(k.index - 1, DEF_BOUNDS) == 0) {
                // check for emptiness, or try to clean up hint
                atomic_deque_node_t *next_left;
                deque_node_t left_peek, left_peek_new;
                
                next_left = (atomic_deque_node_t *) k.nodes[mod(k.index - 1, DEF_BOUNDS)].load().value;
                left_peek = next_left[mod(k.index - 3, DEF_BOUNDS)].load();
                // empty if peek val is null (straddling case)
                if(left_peek.value == LNULL && compare_val(next_left[mod(k.index - 3, DEF_BOUNDS)], left_peek)) {
                    stat = EMPTY;
                    return NULL;
                }
                
                copy_deque_node(left_peek_new, left_peek);
                copy_deque_node(next_new, next);
                
                if(k.nodes[mod(k.index, DEF_BOUNDS)].compare_exchange_strong(next, next_new)) {
                    if(next_left[mod(k.index - 3, DEF_BOUNDS)].compare_exchange_strong(left_peek, left_peek_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, next_left, k.index - 2);
                        deque.right_hint.compare_exchange_strong(k, new_hint);
                    }
                }
            } else {
                if(current.value == LNULL && compare_val(k.nodes[mod(k.index - 1, DEF_BOUNDS)], current)) {
                    stat = EMPTY;
                    return NULL;
                }
                
                // otherwise, do the normal swap op
                set_deque_node(cur_new, RNULL, current.count);
                set_deque_node(next_new, RNULL, next.count);
                
                if(k.nodes[mod(k.index, DEF_BOUNDS)].compare_exchange_strong(next, next_new)) {
                    if(k.nodes[mod(k.index - 1, DEF_BOUNDS)].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, k.nodes, k.index - 1);
                        deque.right_hint.compare_exchange_strong(k, new_hint);
                        deque.size--;
                        stat = OK;
                        return (int *) current.value;
                    }
                }
            }
        }
    }
}

deque_hint_t oracle(deque_t &deque, oracle_end deque_end) {
    deque_hint_t k, new_hint;
    long int i, start;
    atomic_deque_node_t *buffer, *next_buffer;
    deque_node_t current, previous, next;
    
    if(deque_end == LEFT) {
        k = deque.left_hint.load();
        buffer = k.nodes;
        start = k.index;
        current = buffer[mod(k.index, DEF_BOUNDS)].load();
        previous = buffer[mod(k.index + 1, DEF_BOUNDS)].load();
        
        if(current.value == LNULL && previous.value != LNULL) {
            return k;
        } else if(current.value == LNULL && previous.value == LNULL) {
            // go right
            while((void *) buffer != RNULL) {
                for(i = start; mod(i, DEF_BOUNDS) < DEF_BOUNDS - 1; i++) {
                    current = buffer[mod(i, DEF_BOUNDS)].load();
                    previous = buffer[mod(i + 1, DEF_BOUNDS)].load();
                    // buffer edge hit, but this will be head since chain ends
                    if(mod(i + 1, DEF_BOUNDS) == DEF_BOUNDS - 1 && current.value == LNULL && previous.value == RNULL) {
                        set_deque_hint(new_hint, buffer, i);
                        return new_hint;
                    }
                    if(current.value == LNULL && previous.value != LNULL) {
                        set_deque_hint(new_hint, buffer, i);
                        return new_hint;
                    }
                }
                // check if right edge is directly ahead on next buffer;
                // if so we need to straddle buffers
                next_buffer = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
                if((void *) next_buffer != RNULL) {
                    deque_node_t next_current, next_previous;
                    next_previous = next_buffer[0].load();
                    next_current = next_buffer[1].load();
                    if(next_current.value == RNULL && next_previous.value != RNULL) {
                        set_deque_hint(new_hint, buffer, i);
                        return new_hint;
                    }
                }
                // move on to next buffer
                buffer = next_buffer;
                start = i + 1;
            }
            return deque.left_hint.load();
        } else {
            return deque.left_hint.load();
        }
    } else if(deque_end == RIGHT) {
        k = deque.right_hint.load();
        buffer = k.nodes;
        start = k.index;
        current = buffer[mod(k.index, DEF_BOUNDS)].load();
        previous = buffer[mod(k.index - 1, DEF_BOUNDS)].load();
        
        if(current.value == RNULL && previous.value != RNULL) {
            return k;
        } else if(current.value == RNULL && previous.value == RNULL) {
            // go left
            while((void *) buffer != LNULL) {
                for(i = start; mod(i, DEF_BOUNDS) > 0; i--) {
                    current = buffer[mod(i, DEF_BOUNDS)].load();
                    previous = buffer[mod(i - 1, DEF_BOUNDS)].load();
                    // buffer edge hit, but this will be head since chain ends
                    if(mod(i - 1, DEF_BOUNDS) == 0 && current.value == RNULL && previous.value == LNULL) {
                        set_deque_hint(new_hint, buffer, i);
                        return new_hint;
                    }
                    if(current.value == RNULL && previous.value != RNULL) {
                        set_deque_hint(new_hint, buffer, i);
                        return new_hint;
                    }
                }
                // check if right edge is directly ahead on next buffer;
                // if so we need to straddle buffers
                next_buffer = (atomic_deque_node_t *) buffer[0].load().value;
                if((void *) next_buffer != LNULL) {
                    deque_node_t prev_current, prev_previous;
                    prev_previous = next_buffer[DEF_BOUNDS - 1].load();
                    prev_current = next_buffer[DEF_BOUNDS - 2].load();
                    if(prev_current.value == LNULL && prev_previous.value != LNULL) {
                        set_deque_hint(new_hint, buffer, i);
                        return new_hint;
                    }
                }
                // move on to next buffer
                buffer = next_buffer;
                start = i - 1;
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
    init_buffer(buffer, DEF_BOUNDS, SPLIT);
    
    deque.size = 0;
    set_deque_hint(left_hint, buffer, DEF_BOUNDS / 2 - 1);
    set_deque_hint(right_hint, buffer, DEF_BOUNDS / 2);
    deque.left_hint.store(left_hint);
    deque.right_hint.store(right_hint);
}

void init_buffer(atomic_deque_node_t *buffer, int size, buffer_fill fill) {
    int i;
    
    for(i = 0; i < size; i++) {
        switch(fill) {
            case SPLIT:
                if(i < size / 2)
                    init_deque_node(buffer[i], LNULL);
                else 
                    init_deque_node(buffer[i], RNULL);
                break;
            case LBUF:
                if(i < size - 1)
                    init_deque_node(buffer[i], LNULL);
                else
                    init_deque_node(buffer[i], RNULL);
                break;
            case RBUF:
                if(i > 0)
                    init_deque_node(buffer[i], RNULL);
                else
                    init_deque_node(buffer[i], LNULL);
                break;
        }
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

void set_deque_hint(deque_hint_t &hint, atomic_deque_node_t *nodes, long int index) {
    hint.nodes = nodes;
    hint.index = index;
}

void copy_deque_hint(deque_hint_t &new_hint, deque_hint_t &old_hint) {
    new_hint.nodes = old_hint.nodes;
    new_hint.index = old_hint.index;
}
