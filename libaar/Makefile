CFLAGS=-Wall -W -O0 -g -pipe


aar-test: aar.o aar-test.o
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

clean:
	rm -rf *.o aar-test
