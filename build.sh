gcc -g -O2 -pipe -Wall -pthread  `pkg-config dbus-glib-1 --cflags` -c -o mediakeys.o mediakeys.c
gcc -o mediakeys.so mediakeys.o -pthread -shared `pkg-config dbus-glib-1 --libs` -fPIC
