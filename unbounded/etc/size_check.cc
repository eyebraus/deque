
#include <atomic>
#include <stdio.h>
#include <stdlib.h>
//#include "../src/deque.h"

using namespace std;

int main(int argc, char *argv[]) {
    printf("sizeof int: %d bytes\n", sizeof(int));
    printf("sizeof unsigned int: %d bytes\n", sizeof(unsigned int));
    printf("sizeof long int: %d bytes\n", sizeof(long int));
    printf("sizeof unsigned long int: %d bytes\n", sizeof(long unsigned int));
    printf("sizeof long long int: %d bytes\n", sizeof(long long int));
    printf("sizeof unsigned long long int: %d bytes\n", sizeof(long long unsigned int));
    printf("sizeof int *: %d bytes\n", sizeof(int *));
    //printf("sizeof atomic_deque_node_t *: %d bytes\n", sizeof(atomic_deque_node_t *));
    printf("sizeof atomic_ulong: %d bytes\n", sizeof(atomic_ulong));
    printf("sizeof void *: %d bytes\n", sizeof(void *));
    return 0;
}
