packnam: packnam.o
	$(CC) packnam.o -o packnam

%.o: %.c
	$(CC) -Wall -g -c $< -o $@

.PHONY: clean

clean:
	rm -f *.o packnam packnam.exe
