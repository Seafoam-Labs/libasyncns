CFLAGS=-Wall -W -O0 -g -pipe


asyncns-test: asyncns.o asyncns-test.o
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

clean:
	rm -rf *.o asyncns-test
