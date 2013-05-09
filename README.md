An Algorithm for Obstruction-Free Double-Ended Queues
=====================================================
Sean Brennan
------------

### Files ###
* herlihy/ :: a bounded deque implementaiton used as a starting point.
    * Makefile :: for all subdirectories.
    * etc/ :: directory for miscellaneous files related but not crucial to algorithm.
    * exp/ :: code for timing and throughpt experiments.
        * mutex.cc :: timing experiment for nonblocking algorithm + mutexes.
        * nonblock.cc :: timing experiment for purely nonblocking algorithm.
    * src/ :: code for the actual deque algorithm.
        * deque.cc :: contains algorithm for Herlihy bounded deque.
        * deque.h :: structs, enums, and prototypes for deque.cc.
    * test/ :: various testing code for algorithm
        * serial.cc :: sanity testing deque in a non-concurrent setting.
        * concurrent.cc :: lightly tests some boundary cases in a concurrent setting.
        * stress.cc :: aggressively test deque. do massive number of operations and check consistency.
        