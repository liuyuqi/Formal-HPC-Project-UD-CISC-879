CC = gcc
CFLAGS = -g -O0

spec: bigloop.c time.c
	$(CC) $(CFLAGS) bigloop.c time.c -o bigloop
	./bigloop

par: bigloop-omp.c time.c
	$(CC) $(CFLAGS) -fopenmp bigloop-omp.c time.c -o bigloop-omp
	./bigloop-omp

run: bigloop.cvl
	civl run bigloop.cvl

run2: bigloop2.cvl
	civl run bigloop2.cvl

verify: bigloop.cvl
	civl verify bigloop.cvl

verify2: bigloop2.cvl
	civl verify bigloop2.cvl

clean:
	rm bigloop bigloop-omp *.log
