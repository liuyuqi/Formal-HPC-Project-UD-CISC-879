CC = gcc
CFLAGS = -g -O0

bigloop: bigloop.c
	$(CC) $(CFLAGS) bigloop.c -o bigloop
	./bigloop

clean:
	rm bigloop *.log
