build: bmpcli.c
	gcc -Wall -g bmpcli.c functions.c -o bmp -lm

run: bmp
	./bmp

clean:
	rm -rf *.o bmp
