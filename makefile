.DEFAULT_GOAL:=libnss_command.so
PREFIX:=/usr/local
.PHONY: clean install uninstall test
export LD_LIBRARY_PATH:=.

libnss_command.so: nss_command.o
	$(CC) -shared -o $@ -Wl,-soname,libnss_command.so.2 $^
	rm -f libnss_command.so.2
	ln -s $@ libnss_command.so.2

nss_command.o: nss_command.c
	$(CC) -fPIC -o $@ -c $<

tests: tests.o nss_command.o
	$(CXX) -o $@ $^

test: tests
	./tests

clean:
	rm -f *.o *.so *.so.2 tests

uninstall:
	rm -f $(PREFIX)/lib/libnss_command.so $(PREFIX)/lib/libnss_command.so.2

install: libnss_command.so uninstall
	cp libnss_command.so $(PREFIX)/lib/
	cp -d libnss_command.so.2 $(PREFIX)/lib/
