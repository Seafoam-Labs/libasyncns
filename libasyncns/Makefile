CFLAGS=-Wall -W -O0 -g -pipe


asyncns-test: aar.o aar-test.o
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

clean:
	rm -rf *.o asyncns-test
