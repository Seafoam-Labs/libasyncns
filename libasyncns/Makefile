CFLAGS=-Wall -W -O0 -g -pipe


ans-test: aar.o aar-test.o
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

clean:
	rm -rf *.o ans-test
