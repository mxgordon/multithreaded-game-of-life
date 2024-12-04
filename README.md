# Conway's (Multithreaded) Game of Life

## About

Made during CS 3013 (Operating Systems) to learn about semaphores, multithreading, and message passing. This repository includes 2 C++ files. `addem.cpp` which is a simple message-passing and multithreading test, just adding numbers together. And `life.cpp` is Conway's Game of Life. 

In these files, I built a custom message passing system using only `pthread.h` and `semaphore.h`, standard libraries for any POSIX system. The limitation is the board size is not infinite (it's set to the board size in the input file). So it's not a perfect recreation, but functionally is very similar.

## Building

To build both files use `make all`. If you only want to build one, use `make <filename>` ex. `make life.cpp`.

## Using
### `life.cpp`

The game take five parameters, two of which are optional:
 - Number of threads, at least 1, must be less than MAX_THREAD (32)
 - Path to file, "small.txt", "static.txt", and "beacon.txt" are included but feel free to make your own
 - Number of generations to calculate. 
   - Note: the program will automatically stop when all the cells are dead, or nothing is changing anymore
 - Optionally, you can include "print" to print every generation, other wise it will only print the first and last
 - Optionally, yo ucan include "input" which will ask for user input before computing the next generation
    - I recommend include "print" as well otherwise you won't see the generations

### `addem.cpp`

Add 'em take two parameters, the number of threads and the number to sum to.