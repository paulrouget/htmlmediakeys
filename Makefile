all:
	gcc -g -O2 -pipe -Wall -pthread  `pkg-config dbus-glib-1 --cflags` -c -o mediakeys.o mediakeys.c
	gcc -o js/data/mediakeys.so mediakeys.o -pthread -shared `pkg-config dbus-glib-1 --libs` -fPIC
	bash -c "cd js/sdk-1.0b4-modified/ && source bin/activate && cd .. && cfx xpi"
	mv js/mediakeys.xpi .

clean:
	rm -f mediakeys.xpi
	rm -f mediakeys.o
	rm -f js/data/mediakeys.so
