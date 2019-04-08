XORG_SERVER_CFLAGS=$(shell pkg-config --cflags xorg-server)

all: libxf86-vanir-common.so

libxf86-vanir-common.so: xf86-vanir-common.c include/xf86-vanir-common.h
	gcc -Wall -shared -fPIC -DPIC $(XORG_SERVER_CFLAGS) $(CFLAGS) $(LDFLAGS) \
		-o libxf86-vanir-common.so xf86-vanir-common.c

clean:
	rm -f *.so
