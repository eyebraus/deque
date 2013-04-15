
#include <atomic>
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
    atomic_int i;
    i = 5;
    cout << i << endl;
    return 0;
}
