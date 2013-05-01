
#include <map>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

void *dont_do_anything(void *args_void) {
    while(true) ;
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    map<pthread_t, int> thread_id_map;
    pthread_t a, b;
    pthread_create(&a, NULL, &dont_do_anything, NULL);
    pthread_create(&b, NULL, &dont_do_anything, NULL);
    thread_id_map[a] = 1;
    thread_id_map[b] = 2;
    printf("A id: %d, B id: %d\n", thread_id_map[a], thread_id_map[b]);
    exit(0);
}
