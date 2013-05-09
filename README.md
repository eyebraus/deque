An Algorithm for Obstruction-Free Double-Ended Queues
=====================================================
Sean Brennan
------------

### Compiling ###
Navigate to either /herlihy or /unbounded.
Once there, simply run "make" in your favorite shell.

### Executing ###
There are several entry points to see the algorithm at work.

### Files ###
* README.md :: this file.
* writeup.tex :: LaTeX file of final writeup for project.
* citations.bib :: bibliography for writeup.
* herlihy/ :: a bounded deque implementaiton used as a starting point.
    * Makefile :: for all subdirectories.
    * etc/ :: directory for miscellaneous files related but not crucial to algorithm.
    * exp/ :: code for timing and throughput experiments.
        * mutex.cc :: timing experiment for nonblocking algorithm + mutexes.
        * nonblock.cc :: timing experiment for purely nonblocking algorithm.
    * src/ :: code for the actual deque algorithm.
        * deque.cc :: contains algorithm for Herlihy bounded deque.
        * deque.h :: structs, enums, and prototypes for deque.cc.
    * test/ :: various testing code for algorithm
        * serial.cc :: sanity testing deque in a non-concurrent setting.
        * concurrent.cc :: lightly tests some boundary cases in a concurrent setting.
        * stress.cc :: aggressively test deque. do massive number of operations and check consistency.
* unbounded/ :: an unbounded deque implementaiton based on Herlihy.
    * Makefile :: for all subdirectories.
    * etc/ :: directory for miscellaneous files related but not crucial to algorithm.
    * exp/ :: code for timing and throughput experiments.
        * mutex.cc :: timing and throughput experiments for c++std deque + mutexes.
        * nonblock.cc :: timing and throughput experiments for purely nonblocking algorithm.
    * prof/ :: various profiling information.
    * src/ :: code for the actual deque algorithm.
        * deque.cc :: contains algorithm for Herlihy bounded deque.
        * deque.h :: structs, enums, and prototypes for deque.cc.
    * test/ :: various testing code for algorithm
        * serial.cc :: sanity testing deque in a non-concurrent setting.
        * concurrent.cc :: lightly tests some boundary cases in a concurrent setting.
        * stress.cc :: aggressively test deque. do massive number of operations and check consistency.