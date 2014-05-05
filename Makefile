CC = gcc
CFLAGS = -g -O0

bigloop: bigloop.c
	$(CC) $(CFLAGS) bigloop.c -o bigloop
	./bigloop

par: bigloop-omp.c
	$(CC) $(CFLAGS) -fopenmp bigloop-omp.c -o bigloop-omp
	./bigloop-omp

run: bigloop.cvl
	civl run bigloop.cvl

verify: bigloop.cvl
	civl verify bigloop.cvl

clean:
	rm bigloop bigloop-omp *.log
