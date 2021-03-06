
#include <atomic>

#define LNULL (void *) 0b01
#define RNULL (void *) 0b10
#define DEF_BOUNDS 32

using namespace std;

/*
 * Types:
 *     ...
 */

typedef struct deque_node_struct {
    void *value;
    unsigned int count;
} deque_node_t;

typedef atomic<deque_node_t> atomic_deque_node_t;

typedef struct deque_hint_struct {
    atomic_deque_node_t *nodes;
    long int index;
} deque_hint_t;

typedef atomic<deque_hint_t> atomic_deque_hint_t;

typedef struct deque_struct {
    atomic_ulong size;
    atomic_deque_hint_t left_hint;
    atomic_deque_hint_t right_hint;
} deque_t;

typedef enum { LEFT, RIGHT } oracle_end;
typedef enum { OK, EMPTY, FULL } deque_state;
typedef enum { SPLIT, LBUF, RBUF } buffer_fill;

/*
 * Inline Functions ("macros")
 */

static inline bool compare_node(atomic_deque_node_t &a, deque_node_t &b) {
    deque_node_t a_copy = a.load();
    return a_copy.value == b.value && a_copy.count == b.count;
}

static inline bool compare_val(atomic_deque_node_t &a, deque_node_t &b) {
    deque_node_t a_copy = a.load();
    return a_copy.value == b.value;
}

static inline bool is_null(void *v) {
    return v == NULL || v == LNULL || v == RNULL;
}

// ACTUALLY do modular math...
static inline long int mod(long int a, long int b) {
    return (a % b + b) % b;
}

/*
 * Function Prototypes
 */

// stack ops
void left_push(deque_t &deque, int *elt, int &stat);
int *left_pop(deque_t &deque, int &stat);
void right_push(deque_t &deque, int *elt, int &stat);
int *right_pop(deque_t &deque, int &stat);
deque_hint_t left_oracle(deque_t &deque);
deque_hint_t right_oracle(deque_t &deque);

// various helpers
void init_deque_node(atomic_deque_node_t &node);
void init_deque(deque_t &deque);
void init_buffer(atomic_deque_node_t *buffer, int size, buffer_fill fill);
void clear_deque_node(deque_node_t &node);
void clear_deque(deque_t &deque);
void clear_buffer(atomic_deque_node_t *buffer, int size);
void set_deque_node(deque_node_t &node, void *value, unsigned int last_count);
void copy_deque_node(deque_node_t &new_node, deque_node_t &old_node);
void set_deque_hint(deque_hint_t &hint, atomic_deque_node_t *nodes, long int index);
void copy_deque_hint(deque_hint_t &new_hint, deque_hint_t &old_hint);

