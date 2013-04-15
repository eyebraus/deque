
#define LNULL (int *) 0b01
#define RNULL (int *) 0b10
#define DEF_BOUNDS 4096
#define CAS_NODE(p,o,n) (atomic_compare_exchange_strong((atomic_ullong *) (p), (long long *) (o), *(long long *) &(n)))
#define EQL_NODE(p,n) ((*(long long *) &(p)) == (*(long long *) &(n)))
#define IS_NULL(v) ((v) == NULL || (v) == LNULL || (v) == RNULL || (v) == DNULL)
#define VAL_EQL(a,b) ((a)->value == (b)->value)

/*
 * Types:
 *     ...
 */

typedef struct bounded_deque_node_struct {
    int *value;
    unsigned int count;
} bounded_deque_node_t;

typedef struct bounded_deque_struct {
    bounded_deque_node_t nodes[DEF_BOUNDS];
    atomic_ulong size;
    atomic_ulong left_hint;
    atomic_ulong right_hint;
} bounded_deque_t;

typedef struct oracle_result_struct {
    unsigned long long int k;
    bounded_deque_node_t left;
    bounded_deque_node_t right;
} oracle_result_t;

typedef enum { LEFT, RIGHT } oracle_ends;
typedef enum { OK, EMPTY, FULL } deque_state;

/*
 * Function Prototypes
 */

// stack ops
void left_push(bounded_deque_t *deque, int elt, int *stat);
int left_pop(bounded_deque_t *deque, int *stat);
void right_push(bounded_deque_t *deque, int elt, int *stat);
int right_pop(bounded_deque_t *deque, int *stat);
//unsigned long long int oracle(oracle_ends end);

// various helpers
void init_bounded_deque_node(bounded_deque_node_t *node);
void init_bounded_deque(bounded_deque_t *deque);
void init_oracle_result(oracle_result_t *result);
void clear_bounded_deque_node(bounded_deque_node_t *node);
void clear_bounded_deque(bounded_deque_t *deque);
void clear_oracle_result(oracle_result_t *result);
void set_bounded_deque_node(bounded_deque_node_t *node, int *value, unsigned int last_count);
void copy_bounded_deque_node(bounded_deque_node_t *new_node, bounded_deque_node_t *old_node);

