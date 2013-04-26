
#include <atomic>

#define LNULL (int *) 0b01
#define RNULL (int *) 0b10
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

/*
 * Inline Functions ("macros")
 */

static inline bool compare_node(atomic_deque_node_t &a, deque_node_t &b) {
    deque_node_t a_copy = a.load();
    return a_copy.value == b.value && a_copy.count == b.count;
}

static inline bool is_null(int *v) {
    return v == NULL || v == LNULL || v == RNULL;
}

/*
 * Function Prototypes
 */

// stack ops
void left_push(deque_t &deque, int *elt, int &stat);
int *left_pop(deque_t &deque, int &stat);
void right_push(deque_t &deque, int *elt, int &stat);
int *right_pop(deque_t &deque, int &stat);
unsigned long int oracle(deque_t &deque, oracle_end deque_end);

// various helpers
void init_deque_node(atomic_deque_node_t &node);
void init_deque(deque_t &deque);
void clear_deque_node(deque_node_t &node);
void clear_deque(deque_t &deque);
void set_deque_node(deque_node_t &node, int *value, unsigned int last_count);
void copy_deque_node(deque_node_t &new_node, deque_node_t &old_node);
void set_deque_hint(deque_hint_t &hint, atomic_deque_node_t *nodes, long int index);
void copy_deque_hint(deque_hint_t &new_hint, deque_hint_t &old_hint);

