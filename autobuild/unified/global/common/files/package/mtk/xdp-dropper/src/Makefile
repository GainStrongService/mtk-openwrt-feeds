XDP_PROG = xdp_drop

$(XDP_PROG).o: $(XDP_PROG).c
	$(CLANG) -v -O2 -g -target $(CLANG_TARGET) $(BPF_CFLAGS) -c $< -o $@

clean:
	rm -f *.o
