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
	if [ -f client ];then rm client;fi
	if [ -f server ];then rm server;fi

client: client.cc
	$(CC) $(CFLAGS) -o $@ $^ ${PKG-CONFIG}

server: server.cc /usr/local/BumbleBee2/examples-libdc-2/utils/stereohelper.cpp
	$(CC) $(CXXFLAGS) $(SERVER_FLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
