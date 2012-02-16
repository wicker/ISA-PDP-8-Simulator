CFLAGS=-Wall -m32 -g
all:
	cc -g main.c -o main
clean:
	rm -rf tracefile.din main 
