
CFLAGS := -O2 -Wall -ffunction-sections -fdata-sections -ggdb -gdwarf-4
LDFLAGS := -Wl,--gc-sections

# Programs
PROGS := ftsnap ftrestore

all: $(PROGS)

ftcommon.o: ftcommon.c
	$(CC) $(CFLAGS) -MD -c -o $@ $<

ftsnap: ftsnap.o ftcommon.o
	$(CC) $(LDFLAGS) -o $@ $^ -lcrypto

ftsnap.o: ftsnap.c
	$(CC) $(CFLAGS) -MD -c -o $@ $<

ftrestore: ftrestore.o ftcommon.o
	$(CC) $(LDFLAGS) -o $@ $^ -lcrypto

ftrestore.o: ftrestore.c
	$(CC) $(CFLAGS) -MD -c -o $@ $<

clean:
	rm -rf $(PROGS) *.o *.d

-include ftsnap.d ftrestore.d ftcommon.d
