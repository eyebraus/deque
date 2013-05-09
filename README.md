An Algorithm for Obstruction-Free Double-Ended Queues
=====================================================
Sean Brennan
------------

### Compiling ###
Navigate to either /herlihy or /unbounded. Once there, simply run "make" in your favorite shell.

### Executing ###
There are several entry points to see the algorithm at work.

In either /herlihy or /unbounded, you can run any of the test programs. I would recommend test/stress; these are most up-to-date with my current deque spec in both cases, and are more general than test/serial or test/concurrent, which are unit tests.

In /unbounded, you can also run exp/nonblock to see that the deque is working. Options for unbounded/exp/nonblock are as follows:
* t: set the number of pthreads to use
* e: choose which experiment to run ("timing" or "throughput")
* w: choose which workflow to use ("stack", "queue", or "random")
* o: when e = "timing", o represents number of ops all threads perform; when e = "throughput", o represents the number of seconds the program should run in total.

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

### Writeup Revisions ###
There have been several revisions to the project report since last submission: