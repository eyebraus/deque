
#include <atomic>

#define LNULL (int *) 0b01
#define RNULL (int *) 0b10
#define DEF_BOUNDS 64

using namespace std;

/*
 * Types:
 *     ...
 */

typedef struct bounded_deque_node_struct {
    int *value;
    unsigned int count;
} bounded_deque_node_t;

typedef atomic<bounded_deque_node_t> atomic_deque_node_t;

typedef struct bounded_deque_struct {
    atomic_deque_node_t nodes[DEF_BOUNDS];
    atomic_ulong size;
    atomic_ulong left_hint;
    atomic_ulong right_hint;
} bounded_deque_t;

typedef enum { LEFT, RIGHT } oracle_end;
typedef enum { OK, EMPTY, FULL } deque_state;

/*
 * Inline Functions ("macros")
 */

static inline bool cas_node(atomic_deque_node_t &current, bounded_deque_node_t &expected, bounded_deque_node_t &desired) {
    return current.compare_exchange_strong(expected, desired);
}

static inline bool compare_node(atomic_deque_node_t &a, bounded_deque_node_t &b) {
    bounded_deque_node_t a_copy = a.load();
    return a_copy.value == b.value && a_copy.count == b.count;
}

static inline bool is_null(int *v) {
    return v == NULL || v == LNULL || v == RNULL;
}

static inline bool val_eql(bounded_deque_node_t &a, bounded_deque_node_t &b) {
    return a.value == b.value;
}

static inline bool compare_val(atomic_deque_node_t &a, bounded_deque_node_t &b) {
    bounded_deque_node_t a_copy = a.load();
    return a_copy.value == b.value;
}

/*
 * Function Prototypes
 */

// stack ops
void left_push(bounded_deque_t &deque, int *elt, int &stat);
int *left_pop(bounded_deque_t &deque, int &stat);
void right_push(bounded_deque_t &deque, int *elt, int &stat);
int *right_pop(bounded_deque_t &deque, int &stat);
unsigned long int oracle(bounded_deque_t &deque, oracle_end deque_end);

// various helpers
void init_bounded_deque_node(atomic_deque_node_t &node);
void init_bounded_deque(bounded_deque_t &deque);
void clear_bounded_deque_node(bounded_deque_node_t &node);
void clear_bounded_deque(bounded_deque_t &deque);
void set_bounded_deque_node(bounded_deque_node_t &node, int *value, unsigned int last_count);
void copy_bounded_deque_node(bounded_deque_node_t &new_node, bounded_deque_node_t &old_node);

