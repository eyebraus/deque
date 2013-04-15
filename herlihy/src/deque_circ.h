
#define LNULL (int *) 0b01
#define RNULL (int *) 0b10
#define DNULL (int *) 0b11 
#define DEF_BOUNDS 4096
#define CAS_NODE(p,o,n) (__sync_bool_compare_and_swap((long long *) (p), *(long long *) &(o), *(long long *) &(n)))
#define ATOMIC_EQL(p,n) CAS_NODE(p,n,n)
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

/*typedef union bounded_deque_node_union {
    bounded_deque_node_t node;
    unsigned long long cas;
} bounded_deque_node_u;*/

typedef struct bounded_deque_struct {
    bounded_deque_node_t nodes[DEF_BOUNDS];
    unsigned long long int size;
    // ...
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
int left_push(bounded_deque_t *deque, int elt);
int left_pop(bounded_deque_t *deque, int *stat);
oracle_result_t *left_checked_oracle(bounded_deque_t *deque);
int right_push(bounded_deque_t *deque, int elt);
int right_pop(bounded_deque_t *deque, int *stat);
oracle_result_t *right_checked_oracle(bounded_deque_t *deque);
unsigned long long int oracle(oracle_ends end);

// various helpers
void init_bounded_deque_node(bounded_deque_node_t *node);
void init_bounded_deque(bounded_deque_t *deque);
void init_oracle_result(oracle_result_t *result);
void clear_bounded_deque_node(bounded_deque_node_t *node);
void clear_bounded_deque(bounded_deque_t *deque);
void clear_oracle_result(oracle_result_t *result);
void set_bounded_deque_node(bounded_deque_node_t *node, int *value, unsigned int last_count);
void copy_bounded_deque_node(bounded_deque_node_t *new_node, bounded_deque_node_t *old_node);

