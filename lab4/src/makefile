CC=gcc
CFLAGS=-I../../lab3/src
PTHREAD_FLAGS=-pthread

all: parallel_min_max process_memory parallel_sum

parallel_min_max : ../../lab3/src/parallel_min_max.c ../../lab3/src/find_min_max.c ../../lab3/src/utils.c
	$(CC) -o parallel_min_max ../../lab3/src/parallel_min_max.c ../../lab3/src/find_min_max.c ../../lab3/src/utils.c $(CFLAGS)

process_memory : process_memory.c
	$(CC) -o process_memory process_memory.c

parallel_sum : parallel_sum.c sum_lib.c ../../lab3/src/utils.c
	$(CC) -o parallel_sum parallel_sum.c sum_lib.c ../../lab3/src/utils.c $(CFLAGS) $(PTHREAD_FLAGS)

clean :
	rm -f parallel_min_max process_memory parallel_sum
