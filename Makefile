CC := g++ 
#CFLAGS := -g
CFLAGS := -g -Wno-deprecated-declarations -Werror

TRICLOPSDIR = /usr/local/triclops
SERVER_FLAGS += -I.
SERVER_FLAGS += -I/usr/local/include
SERVER_FLAGS += -I/usr/local/BumbleBee2/examples-libdc-2/utils
SERVER_FLAGS += -I$(TRICLOPSDIR)/include
SERVER_FLAGS += -Wall -g
SERVER_FLAGS += -DLINUX

LDFLAGS += -L. -L$(TRICLOPSDIR)/lib
LIBS    += -ldc1394 -lraw1394 -pthread
LIBS    += -ltriclops -lpnmutils

PKG-CONFIG := `pkg-config --cflags --libs opencv`

clean:
	rm ./client ./server *.o 

client: client.cc
	$(CC) $(CFLAGS) -o $@ $^ ${PKG-CONFIG}

server: server.cc /usr/local/BumbleBee2/examples-libdc-2/utils/stereohelper.cpp
	$(CC) $(SERVER_FLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)