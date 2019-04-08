CC ?= gcc
CFLAGS += -I../include/ `pkg-config --cflags vchan-$(BACKEND_VMM)` -g -Wall -Wextra -Werror -pie -fPIC
OBJS = vmside.o \
	../gui-common/txrx-vchan.o ../gui-common/error.o ../common/list.o 
LIBS = -lX11 -lXdamage -lXcomposite `pkg-config --libs vchan-$(BACKEND_VMM)` -lvanirdb


all: vanir-gui
vanir-gui: $(OBJS)
	$(CC) $(LDFLAGS) -pie -g -o vanir-gui $(OBJS) \
		$(LIBS)
clean:
	rm -f vanir-gui *.o *~
