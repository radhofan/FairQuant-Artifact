CC=gcc
INSTALL_PREFIX=$(HOME)/OpenBLAS
# or wherever you installed OpenBLAS

CFLAGS = -I $(INSTALL_PREFIX)/include
LDFLAGS = -L $(INSTALL_PREFIX)/lib -lopenblas -lm

all: network_test 
all: CFLAGS += -O3
all: LDFLAGS += -O3

debug: network_test
debug: CFLAGS += -DDEBUG -g
debug: LDFLAGS += -DDEBUG -g

network_test: matrix.o nnet.o network_test.o split.o prop.o
	$(CC) -o $@ $^ $(LDFLAGS)

c.o: 
	$(CC) $(CFLAGS) $< -o $@

clean_obj:
	rm -f *.o

clean_all:
	rm -f *.o network_test

