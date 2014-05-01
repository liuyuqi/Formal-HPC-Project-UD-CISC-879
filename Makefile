CC = gcc
CFLAGS = -g -O0

onetime: onetime.c
	$(CC) $(CFLAGS) onetime.c -o onetime
	./onetime

bigloop: bigloop.c
	$(CC) $(CFLAGS) bigloop.c -o bigloop
	./bigloop

clean:
	rm onetime bigloop *.log
