LIB=-lpthread
CC=gcc
CCPP=g++
FLAGS=

all: addem life

addem: addem.cpp
	 $(CCPP) addem.cpp -o addem $(LIB) $(FLAGS)

life: life.cpp
	 $(CCPP) life.cpp -o life $(LIB) $(FLAGS)

clean: 
	/bin/rm -f *.o core addem life
