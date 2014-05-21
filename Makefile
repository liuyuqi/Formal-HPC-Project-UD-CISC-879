CC = gcc
CFLAGS = -g -O0

spec: bigloop.c time.c
	$(CC) $(CFLAGS) bigloop-omp.c time.c -o bigloop
	./bigloop

par: bigloop-omp.c time.c
	$(CC) $(CFLAGS) -fopenmp bigloop-omp.c time.c -o bigloop-omp
	./bigloop-omp

run: bigloop.cvl
	civl run bigloop.cvl

verify: bigloop.cvl
	civl verify bigloop.cvl

clean:
	rm bigloop bigloop-omp *.log
