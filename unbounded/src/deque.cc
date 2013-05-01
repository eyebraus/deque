
#include <atomic>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
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
        hazard_mark(deque, k.nodes, CURRENT); // mark buffer as hazardous
        previous = k.nodes[mod(k.index + 1, DEF_BOUNDS)].load();
        current = k.nodes[mod(k.index, DEF_BOUNDS)].load();
        
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
                        hazard_retire(deque, k.nodes, CURRENT); // release hazard
                        return;
                    } else {
                        // make sure to free unused buffer!
                        hazard_retire(deque, k.nodes, CURRENT); // release hazard
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
                        hazard_retire(deque, k.nodes, CURRENT); // release hazard
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
        hazard_mark(deque, k.nodes, CURRENT); // mark buffer as hazardous
        current = k.nodes[mod(k.index + 1, DEF_BOUNDS)].load();
        next = k.nodes[mod(k.index, DEF_BOUNDS)].load();
        
        if(current.value != LNULL && next.value == LNULL) {
            deque_node_t cur_new, next_new;
            
            if(current.value != RNULL && mod(k.index + 1, DEF_BOUNDS) == DEF_BOUNDS - 1) {
                // cleanup case: try to detach and reclaim buffer
                atomic_deque_node_t *next_right;
                deque_node_t right_ptr_new, left_ptr_new, left_ptr_old, left_peek;
                
                // set sail for right neighbor!
                // TODO: is it possible to fall asleep here and wake up with
                // ptr to next buffer freed?
                next_right = (atomic_deque_node_t *) k.nodes[mod(k.index + 1, DEF_BOUNDS)].load().value;
                hazard_mark(deque, next_right, NEXT); // mark buffer as hazardous
                left_ptr_old = next_right[mod(k.index + 2, DEF_BOUNDS)].load();
                // if next buffer no longer points to this one, return immediately
                if((atomic_deque_node_t *) left_ptr_old.value != k.nodes) {
                    hazard_retire(deque, k.nodes, CURRENT); // release hazard
                    hazard_retire(deque, next_right, NEXT); // release hazard
                    continue;
                }
                left_peek = next_right[mod(k.index + 3, DEF_BOUNDS)].load();
                // empty if peek val is null (straddling case)
                if(left_peek.value == RNULL && compare_val(next_right[mod(k.index + 3, DEF_BOUNDS)], left_peek)) {
                    stat = EMPTY;
                    hazard_retire(deque, k.nodes, CURRENT); // release hazard
                    hazard_retire(deque, next_right, NEXT); // release hazard
                    return NULL;
                }
                
                // attempt to detach
                set_deque_node(next_new, LNULL, next.count);
                set_deque_node(left_ptr_new, LNULL, left_ptr_old.count);
                // ASSUMPTION! we don't care that the right pointer can change
                // if it points to a new buffer, then next would have had to
                // change as well
                set_deque_node(cur_new, RNULL, current.count);
                
                if(k.nodes[mod(k.index, DEF_BOUNDS)].compare_exchange_strong(next, next_new)) {
                    if(next_right[mod(k.index + 2, DEF_BOUNDS)].compare_exchange_strong(left_ptr_old, left_ptr_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, next_right, k.index + 2);
                        deque.left_hint.compare_exchange_strong(k, new_hint);
                        k.nodes[mod(k.index + 1, DEF_BOUNDS)].store(cur_new);
                        hazard_retire(deque, k.nodes, CURRENT); // release hazard
                        hazard_retire(deque, next_right, NEXT); // release hazard
                    }
                }
            } else {
                if(current.value == RNULL && compare_val(k.nodes[mod(k.index + 1, DEF_BOUNDS)], current)) {
                    stat = EMPTY;
                    hazard_retire(deque, k.nodes, CURRENT);
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
                        hazard_retire(deque, k.nodes, CURRENT); // release hazard
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
        hazard_mark(deque, k.nodes, CURRENT); // mark buffer as hazardous
        previous = k.nodes[mod(k.index - 1, DEF_BOUNDS)].load();
        current = k.nodes[mod(k.index, DEF_BOUNDS)].load();
        
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
                        hazard_retire(deque, k.nodes, CURRENT); // release hazard
                        return;
                    } else {
                        // make sure to free unused buffer!
                        hazard_retire(deque, k.nodes, CURRENT); // release hazard
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
                        hazard_retire(deque, k.nodes, CURRENT); // release hazard
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
        hazard_mark(deque, k.nodes, CURRENT);
        current = k.nodes[mod(k.index - 1, DEF_BOUNDS)].load();
        next = k.nodes[mod(k.index, DEF_BOUNDS)].load();
        
        if(current.value != RNULL && next.value == RNULL) {
            deque_node_t cur_new, next_new;
            
            if(current.value != LNULL && mod(k.index - 1, DEF_BOUNDS) == 0) {
                // cleanup case: try to detach and reclaim buffer
                // TODO: hazard pointers
                atomic_deque_node_t *next_left;
                deque_node_t left_ptr_new, right_ptr_new, right_ptr_old, right_peek;
                
                // set sail for right neighbor!
                next_left = (atomic_deque_node_t *) k.nodes[mod(k.index - 1, DEF_BOUNDS)].load().value;
                hazard_mark(deque, next_left, NEXT);
                right_ptr_old = next_left[mod(k.index - 2, DEF_BOUNDS)].load();
                // if next buffer no longer points to this one, return immediately
                if((atomic_deque_node_t *) right_ptr_old.value != k.nodes) {
                    hazard_retire(deque, k.nodes, CURRENT);
                    hazard_retire(deque, next_left, NEXT);
                    continue;
                }
                right_peek = next_left[mod(k.index - 3, DEF_BOUNDS)].load();
                // empty if peek val is null
                if(right_peek.value == LNULL && compare_val(next_left[mod(k.index - 3, DEF_BOUNDS)], right_peek)) {
                    stat = EMPTY;
                    hazard_retire(deque, k.nodes, CURRENT);
                    hazard_retire(deque, next_left, NEXT);
                    return NULL;
                }
                
                // attempt to detach
                set_deque_node(next_new, RNULL, next.count);
                set_deque_node(right_ptr_new, RNULL, right_ptr_old.count);
                // ASSUMPTION! we don't care that the right pointer can change
                // if it points to a new buffer, then next would have had to
                // change as well
                set_deque_node(cur_new, LNULL, current.count);
                
                if(k.nodes[mod(k.index, DEF_BOUNDS)].compare_exchange_strong(next, next_new)) {
                    if(next_left[mod(k.index - 2, DEF_BOUNDS)].compare_exchange_strong(right_ptr_old, right_ptr_new)) {
                        // update loc hint
                        deque_hint_t new_hint;
                        set_deque_hint(new_hint, next_left, k.index - 2);
                        deque.right_hint.compare_exchange_strong(k, new_hint);
                        k.nodes[mod(k.index - 1, DEF_BOUNDS)].store(cur_new);
                        hazard_retire(deque, k.nodes, CURRENT);
                        hazard_retire(deque, next_left, NEXT);
                    }
                }
            } else {
                if(current.value == LNULL && compare_val(k.nodes[mod(k.index - 1, DEF_BOUNDS)], current)) {
                    stat = EMPTY;
                    hazard_retire(deque, k.nodes, CURRENT);
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
                        hazard_retire(deque, k.nodes, CURRENT);
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

void hazard_mark(deque_t &deque, atomic_deque_node_t *ptr, hazard_type type) {
    pthread_t me = pthread_self();
    
    // immediately remove ptr from retired list if present    
    if(deque.hazards[me]->retired.find(ptr) != deque.hazards[me]->retired.end())
        deque.hazards[me]->retired.erase(ptr);
    
    if(type == CURRENT) {
        deque.hazards[me]->left = ptr;
    } else {
        deque.hazards[me]->right = ptr;
    }
}

void hazard_retire(deque_t &deque, atomic_deque_node_t *ptr, hazard_type type) {
    pthread_t me = pthread_self();
    
    if(type == CURRENT && deque.hazards[me]->left == ptr)
        deque.hazards[me]->retired.insert(ptr);
    if(type == NEXT && deque.hazards[me]->right == ptr)
        deque.hazards[me]->retired.insert(ptr);
    
    if(deque.hazards[me]->retired.size() >= RETIRE_TICK) {
        hazard_scan(deque);
    }
}

void hazard_scan(deque_t &deque) {
    set<atomic_deque_node_t *> plist;
    set<atomic_deque_node_t *> rlist;
    set<atomic_deque_node_t *>::iterator r;
    map<pthread_t, hazard_t *>::iterator m;
    pthread_t me = pthread_self();
    
    // stage 1: scan all ptrs and insert into removal set
    for(m = deque.hazards.begin(); m != deque.hazards.end(); m++) {
        hazard_t *hazard = m->second;
        if(hazard->left != NULL)
            plist.insert(hazard->left);
        if(hazard->right != NULL)
            plist.insert(hazard->right);
    }
    
    // stage 2: search removal set and free
    rlist = deque.hazards[me]->retired; // local copy
    for(r = rlist.begin(); r != rlist.end(); r++) {
        deque.hazards[me]->retired.erase(*r);
        if(plist.find(*r) == plist.end()) {
            // unused buffer, can be cleared and freed!
            clear_buffer(*r, DEF_BOUNDS);
            free(*r);
        } else {
            // some other thread is still using this pointer
            deque.hazards[me]->retired.insert(*r);
        }
    }
}

void init_deque_node(atomic_deque_node_t &node, void *init_null) {
    deque_node_t blank_node;
    blank_node.value = init_null;
    blank_node.count = 0;
    node.store(blank_node);
}

void init_deque(deque_t &deque, pthread_t *threads, int thread_count) {
    atomic_deque_node_t *buffer;
    deque_hint_t left_hint, right_hint;
    int i;
    
    buffer = (atomic_deque_node_t *) malloc(DEF_BOUNDS * sizeof(atomic_deque_node_t));
    init_buffer(buffer, DEF_BOUNDS, SPLIT);
    
    deque.size = 0;
    set_deque_hint(left_hint, buffer, DEF_BOUNDS / 2 - 1);
    set_deque_hint(right_hint, buffer, DEF_BOUNDS / 2);
    deque.left_hint.store(left_hint);
    deque.right_hint.store(right_hint);
    
    for(i = 0; i < thread_count; i++) {
        hazard_t *hazard = (hazard_t *) malloc(sizeof(hazard_t));
        init_hazard(hazard);
        deque.hazards[threads[i]] = hazard;
    }
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

void init_hazard(hazard_t *hazard) {
    hazard->left = NULL;
    hazard->right = NULL;
    hazard->retired = set<atomic_deque_node_t *>();
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
    map<pthread_t, hazard_t *>::iterator iter;
    set<atomic_deque_node_t *> freed;
    
    buffer = deque.left_hint.load().nodes;
    left_next = (atomic_deque_node_t *) buffer[0].load().value;
    right_next = (atomic_deque_node_t *) buffer[DEF_BOUNDS - 1].load().value;
    clear_buffer(buffer, DEF_BOUNDS);
    
    left_check = !is_null(left_next);
    bufsave = buffer;
    while(!is_null(left_next) && !is_null(right_next)) {
        if(left_check) {
            buffer = left_next;
            freed.insert(buffer);
            left_next = (atomic_deque_node_t *) buffer[0].load().value;
            clear_buffer(buffer, DEF_BOUNDS);
            // free!
            free(buffer);
        } else {
            buffer = right_next;
            freed.insert(buffer);
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
    
    for(iter = deque.hazards.begin(); iter != deque.hazards.end(); iter++) {
        set<atomic_deque_node_t *>::iterator v;
        atomic_deque_node_t *left = deque.hazards[iter->first]->left;
        atomic_deque_node_t *right = deque.hazards[iter->first]->right;
        
        if(freed.find(left) == freed.end()) {
            clear_buffer(left, DEF_BOUNDS);
            free(left);
            freed.insert(left);
            deque.hazards[iter->first]->left = NULL;
        }
        
        if(freed.find(right) == freed.end()) {
            clear_buffer(right, DEF_BOUNDS);
            free(right);
            freed.insert(right);
            deque.hazards[iter->first]->right = NULL;
        }
        
        for(v = iter->second->retired.begin(); v != iter->second->retired.end(); v++) {
            if(freed.find(*v) == freed.end()) {
                clear_buffer(*v, DEF_BOUNDS);
                free(*v);
                freed.insert(*v);
            }
        }
        iter->second->retired.clear();
    }
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
