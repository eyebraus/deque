
#include <stdio.h>
#include <stdlib.h>
#include "deque.h"

int main(int argc, char *argv[]) {
    bounded_deque_node_t lnull, rnull, dnull;
    lnull.value = LNULL;
    rnull.value = RNULL;
    dnull.value = DNULL;
    printf("At LNULL: %lu\n", *lnull.value);
    printf("At RNULL: %lu\n", *rnull.value);
    printf("At DNULL: %lu\n", *dnull.value);
    return 0;
}
