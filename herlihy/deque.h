
#define LNULL 0b01
#define RNULL 0b10
#define DNULL 0b11 
#define DEF_BOUNDS 4096

/*
 * Types:
 *     ...
 */

typedef struct bounded_deque_node_struct {
    double *value;
} bounded_deque_node_t;

typedef struct bounded_deque_struct {
    bounded_deque_node_t nodes[DEF_BOUNDS];
    unsigned long long int size;
    // ...
} bounded_deque_t;

typedef enum { LEFT, RIGHT } oracle_ends;

/*
 * Function Prototypes
 */

void left_push(bounded_queue_t *deque, double *elt);
double *left_pop(bounded_queue_t *deque);
unsigned long long int left_checked_oracle(bounded_queue_t *deque);
void right_push(bounded_queue_t *deque, double *elt);
double *right_pop(bounded_queue_t *deque);
unsigned long long int right_checked_oracle(bounded_queue_t *deque);
unsigned long long int oracle(oracle_ends end);
