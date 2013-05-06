
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
    long int kprev, kcur;
    deque_node_t previous, current;
    bool dumbprint = true;
    
    while(1) {
        k = oracle(deque, LEFT);
        kprev = mod(k.index + 1, DEF_BOUNDS);
        kcur = mod(k.index, DEF_BOUNDS);
        previous = k.nodes[kprev].load();
        current = k.nodes[kcur].load();
        
        if(previous.value != LNULL && (current.value == LNULL || kcur == 0)) {
            deque_node_t prev_new, cur_new;
            
            if(kcur == 0 && current.value == LNULL) {
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
                
                if(k.nodes[kprev].compare_exchange_strong(previous, prev_new)) {
                    if(k.nodes[kcur].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, buffer, k.index - 3);
                        deque.left_hint.store(new_hint);
                        deque.size++;
                        stat = OK;
                        return;
                    } else {
                        // make sure to free unused buffer!
                        free(buffer);
                    }
                } else {
                    free(buffer);
                }
            } else if(kcur == 0 && current.value != LNULL) {
                if(dumbprint) {
                    fprintf(stdout, "\t\t\t\tLeft push says hi\n");
                    dumbprint = false;
                }
                // new buffer already allocated, use it!
                atomic_deque_node_t *buffer;
                long int kpeek = mod(k.index - 2, DEF_BOUNDS);
                deque_node_t left_peek, left_peek_new;
                
                buffer = (atomic_deque_node_t *) current.value;
                left_peek = buffer[kpeek].load();
                
                copy_deque_node(prev_new, previous);
                set_deque_node(left_peek_new, (void *) elt, left_peek.count);
                
                if(k.nodes[kprev].compare_exchange_strong(previous, prev_new)) {
                    if(buffer[kpeek].compare_exchange_strong(left_peek, left_peek_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, buffer, k.index - 3);
                        deque.left_hint.store(new_hint);
                        deque.size++;
                        stat = OK;
                        return;
                    }
                }
            } else {
                // otherwise do the normal swapping case
                copy_deque_node(prev_new, previous);
                set_deque_node(cur_new, (void *) elt, current.count);
                
                if(k.nodes[kprev].compare_exchange_strong(previous, prev_new)) {
                    if(k.nodes[kcur].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, k.nodes, k.index - 1);
                        deque.left_hint.store(new_hint);
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
    long int kcur, knext;
    deque_node_t current, next;
    
    while(1) {
        k = oracle(deque, LEFT);
        kcur = mod(k.index + 1, DEF_BOUNDS);
        knext = mod(k.index, DEF_BOUNDS);
        current = k.nodes[kcur].load();
        next = k.nodes[knext].load();
        
        if(current.value != LNULL && (next.value == LNULL || knext == 0)) {
            deque_node_t cur_new, next_new;
            
            if(current.value != RNULL && kcur == DEF_BOUNDS - 1) {
                // check for emptiness, or try to clean up hint
                atomic_deque_node_t *next_right;
                long int kpeek = mod(k.index + 3, DEF_BOUNDS);
                deque_node_t right_peek, right_peek_new;
                
                next_right = (atomic_deque_node_t *) k.nodes[kcur].load().value;
                right_peek = next_right[kpeek].load();
                // restart if peek val is LNULL (someone already did this pop)
                if(right_peek.value == LNULL)
                    continue;
                // empty if peek val is RNULL (straddling case)
                if(right_peek.value == RNULL && compare_node(next_right[kpeek], right_peek)) {
                    stat = EMPTY;
                    return NULL;
                }
                
                set_deque_node(right_peek_new, LNULL, right_peek.count);
                set_deque_node(next_new, LNULL, next.count);
                
                if(k.nodes[knext].compare_exchange_strong(next, next_new)) {
                    if(next_right[kpeek].compare_exchange_strong(right_peek, right_peek_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, next_right, k.index + 3);
                        deque.left_hint.store(new_hint);
                        deque.size--;
                        stat = OK;
                        return (int *) right_peek.value;
                    }
                }
            } else {
                if(current.value == RNULL && compare_node(k.nodes[kcur], current)) {
                    stat = EMPTY;
                    return NULL;
                }
                
                // otherwise, do the normal swap op
                set_deque_node(cur_new, LNULL, current.count);
                copy_deque_node(next_new, next);
                
                if(k.nodes[knext].compare_exchange_strong(next, next_new)) {
                    if(k.nodes[kcur].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, k.nodes, k.index + 1);
                        deque.left_hint.store(new_hint);
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
    long int kprev, kcur;
    deque_node_t previous, current;
    bool dumbprint = true;
    
    while(1) {
        k = oracle(deque, RIGHT);
        kprev = mod(k.index - 1, DEF_BOUNDS);
        kcur = mod(k.index, DEF_BOUNDS);
        previous = k.nodes[kprev].load();
        current = k.nodes[kcur].load();
        
        // TODO: FIX THIS!!!! you should push whenever current is at SIZE - 1
        if(previous.value != RNULL && (current.value == RNULL || kcur == DEF_BOUNDS - 1)) {
            deque_node_t prev_new, cur_new;
            
            if(kcur == DEF_BOUNDS - 1 && current.value == RNULL) {
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
                
                if(k.nodes[kprev].compare_exchange_strong(previous, prev_new)) {
                    if(k.nodes[kcur].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, buffer, k.index + 3);
                        deque.right_hint.store(new_hint);
                        deque.size++;
                        stat = OK;
                        return;
                    } else {
                        // make sure to free unused buffer!
                        free(buffer);
                    }
                } else {
                    free(buffer);
                }
            } else if(kcur == DEF_BOUNDS - 1 && current.value != RNULL) {
                if(dumbprint) {
                    fprintf(stdout, "\t\t\t\tRight push says hi\n");
                    dumbprint = false;
                }
                // new buffer already allocated, use it!
                atomic_deque_node_t *buffer;
                long int kpeek = mod(k.index + 2, DEF_BOUNDS);
                deque_node_t right_peek, right_peek_new;
                
                buffer = (atomic_deque_node_t *) k.nodes[kcur].load().value;
                right_peek = buffer[kpeek].load();
                
                copy_deque_node(prev_new, previous);
                set_deque_node(right_peek_new, (void *) elt, right_peek.count);
                
                if(k.nodes[kprev].compare_exchange_strong(previous, prev_new)) {
                    if(buffer[kpeek].compare_exchange_strong(right_peek, right_peek_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, buffer, k.index + 3);
                        deque.right_hint.store(new_hint);
                        deque.size++;
                        stat = OK;
                        return;
                    }
                }
            } else {
                // otherwise do the normal swapping case
                copy_deque_node(prev_new, previous);
                set_deque_node(cur_new, (void *) elt, current.count);
                
                if(k.nodes[kprev].compare_exchange_strong(previous, prev_new)) {
                    if(k.nodes[kcur].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, k.nodes, k.index + 1);
                        deque.right_hint.store(new_hint);
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
    long int kcur, knext;
    deque_node_t current, next;
    
    while(1) {
        k = oracle(deque, RIGHT);
        kcur = mod(k.index - 1, DEF_BOUNDS);
        knext = mod(k.index, DEF_BOUNDS);
        current = k.nodes[kcur].load();
        next = k.nodes[knext].load();
        
        if(current.value != RNULL && (next.value == RNULL || knext == DEF_BOUNDS - 1)) {
            deque_node_t cur_new, next_new;
            
            // TODO: I think only second half of && is needed
            if(current.value != LNULL && kcur == 0) {
                // check for emptiness, or try to clean up hint
                atomic_deque_node_t *next_left;
                long int kpeek = mod(k.index - 3, DEF_BOUNDS);
                deque_node_t left_peek, left_peek_new;
                
                next_left = (atomic_deque_node_t *) k.nodes[kcur].load().value;
                left_peek = next_left[kpeek].load();
                // restart if peek val is RNULL (someone already did this pop)
                if(left_peek.value == RNULL)
                    continue;
                // empty if peek val is null (straddling case)
                if(left_peek.value == LNULL && compare_node(next_left[kpeek], left_peek)) {
                    stat = EMPTY;
                    return NULL;
                }
                
                set_deque_node(left_peek_new, RNULL, left_peek.count);
                set_deque_node(next_new, RNULL, next.count);
                
                if(k.nodes[knext].compare_exchange_strong(next, next_new)) {
                    if(next_left[kpeek].compare_exchange_strong(left_peek, left_peek_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, next_left, k.index - 3);
                        deque.right_hint.store(new_hint);
                        deque.size--;
                        stat = OK;
                        return (int *) left_peek.value;
                    }
                }
            } else {
                if(current.value == LNULL && compare_node(k.nodes[kcur], current)) {
                    stat = EMPTY;
                    return NULL;
                }
                
                // otherwise, do the normal swap op
                set_deque_node(cur_new, RNULL, current.count);
                copy_deque_node(next_new, next);
                
                if(k.nodes[knext].compare_exchange_strong(next, next_new)) {
                    if(k.nodes[kcur].compare_exchange_strong(current, cur_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, k.nodes, k.index - 1);
                        deque.right_hint.store(new_hint);
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
    if(deque_end == LEFT) {
        return left_oracle(deque);
    } else if(deque_end == RIGHT) {
        return right_oracle(deque);
    }
}

deque_hint_t left_oracle(deque_t &deque) {
    deque_hint_t k, new_hint;
    long int i, index, kcur, kprev;
    atomic_deque_node_t *buffer, *next_buffer;
    deque_node_t current, previous, next;

    k = deque.left_hint.load();
    buffer = k.nodes;
    index = k.index;
    kcur = mod(k.index, DEF_BOUNDS);
    kprev = mod(k.index + 1, DEF_BOUNDS);
    current = buffer[kcur].load();
    previous = buffer[kprev].load();

    // find leftmost point then work inward
    next_buffer = (atomic_deque_node_t *) buffer[0].load().value;
    while((void *) next_buffer != LNULL) {
        buffer = next_buffer;
        next_buffer = (atomic_deque_node_t *) buffer[0].load().value;
        index -= DEF_BOUNDS;
    }
    index -= mod(index, DEF_BOUNDS);
    
    while((void *) buffer != RNULL) {
        for(i = index; mod(i, DEF_BOUNDS) < DEF_BOUNDS - 1; i++) {
            kcur = mod(i, DEF_BOUNDS);
            kprev = mod(i + 1, DEF_BOUNDS);
            current = buffer[kcur].load();
            previous = buffer[kprev].load();
            // buffer edge hit, but this will be head since chain ends
            if(kprev == DEF_BOUNDS - 1 && current.value == LNULL && previous.value == RNULL) {
                set_deque_hint(new_hint, buffer, index + mod(i, DEF_BOUNDS));
                return new_hint;
            }
            if(kprev != DEF_BOUNDS - 1 && (kcur == 0 || current.value == LNULL) && previous.value != LNULL) {
                set_deque_hint(new_hint, buffer, index + mod(i, DEF_BOUNDS));
                return new_hint;
            }
        }
        // check if right edge is directly ahead on next buffer;
        // if so we need to straddle buffers
        next_buffer = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
        if((void *) next_buffer != RNULL) {
            deque_node_t next_current, next_previous;
            next_previous = next_buffer[1].load();
            next_current = next_buffer[0].load();
            if(next_previous.value == RNULL && next_current.value != RNULL) {
                set_deque_hint(new_hint, buffer, index + DEF_BOUNDS - 2);
                return new_hint;
            }
        }
        // move on to next buffer
        buffer = next_buffer;
        index += DEF_BOUNDS;
    }
    
    return k;
}

deque_hint_t right_oracle(deque_t &deque) {
    deque_hint_t k, new_hint;
    long int i, index, kcur, kprev;
    atomic_deque_node_t *buffer, *next_buffer;
    deque_node_t current, previous, next;

    k = deque.right_hint.load();
    buffer = k.nodes;
    index = k.index;
    kcur = mod(k.index, DEF_BOUNDS);
    kprev = mod(k.index - 1, DEF_BOUNDS);
    current = buffer[kcur].load();
    previous = buffer[kprev].load();

    // find rightmost point then work inward
    next_buffer = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
    while((void *) next_buffer != RNULL) {
        buffer = next_buffer;
        next_buffer = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
        index += DEF_BOUNDS;
    }
    index -= mod(index, DEF_BOUNDS);
    
    while((void *) buffer != LNULL) {
        for(i = index + DEF_BOUNDS - 1; mod(i, DEF_BOUNDS) > 0; i--) {
            kcur = mod(i, DEF_BOUNDS);
            kprev = mod(i - 1, DEF_BOUNDS);
            current = buffer[kcur].load();
            previous = buffer[kprev].load();
            // buffer edge hit, but this will be head since chain ends
            if(kprev == 0 && current.value == RNULL && previous.value == LNULL) {
                set_deque_hint(new_hint, buffer, index + mod(i, DEF_BOUNDS));
                return new_hint;
            }
            if(kprev != 0 && (kcur == DEF_BOUNDS - 1 || current.value == RNULL) && previous.value != RNULL) {
                set_deque_hint(new_hint, buffer, index + mod(i, DEF_BOUNDS));
                return new_hint;
            }
        }
        // check if right edge is directly ahead on next buffer;
        // if so we need to straddle buffers
        next_buffer = (atomic_deque_node_t *) buffer[0].load().value;
        if((void *) next_buffer != LNULL) {
            deque_node_t next_current, next_previous;
            next_previous = next_buffer[DEF_BOUNDS - 2].load();
            next_current = next_buffer[DEF_BOUNDS - 1].load();
            if(next_previous.value == LNULL && next_current.value != LNULL) {
                set_deque_hint(new_hint, buffer, index + 1);
                return new_hint;
            }
        }
        // move on to next buffer
        buffer = next_buffer;
        index -= DEF_BOUNDS;
    }
    
    return k;
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
    //if(!is_null(old_node.value))
    //    free(old_node.value);
}

void clear_deque(deque_t &deque) {
    bool left_check;
    atomic_deque_node_t *buffer, *bufsave, *left_next, *right_next;
    deque_hint_t left_hint, right_hint;
    
    buffer = deque.left_hint.load().nodes;
    left_next = (atomic_deque_node_t *) buffer[0].load().value;
    right_next = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
    clear_buffer(buffer, DEF_BOUNDS);
    init_buffer(buffer, DEF_BOUNDS, SPLIT);
    
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
