CFLAGS  :=-m32 -I. -Iosx -std=gnu11 -O2 -g -Wall -fdiagnostics-color=always -Wno-unused-variable -Wno-unused-function -march=native -mtune=native -pthread -fno-strict-aliasing
CXXFLAGS:=-m32 -I. -Iosx -std=gnu++14 -O2 -g -Wall -fdiagnostics-color=always -Wno-unused-variable -Wno-unused-function -march=native -mtune=native -pthread -fno-strict-aliasing
LDFLAGS :=-Wl,-rpath=. -lm -ldl -lbsd -lelf -Wl,-E libiberty.a

SOURCES_C  :=$(shell find . -follow -type f -name '*.c')
SOURCES_CXX:=$(shell find . -follow -type f -name '*.cc')
OBJECTS    :=$(patsubst %.cc,%.o,$(SOURCES_CXX)) $(patsubst %.c,%.o,$(SOURCES_C))
HEADERS    :=$(shell find . -follow -type f -name '*.h')

.PHONY: all clean

all: classgraph

clean:
	rm -f classgraph $(shell find . -iname '*.o')

classgraph: $(OBJECTS) Makefile
	g++ $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

%.o: %.cc $(HEADERS) Makefile
	g++ $(CXXFLAGS) -o $@ -c $<

%.o: %.c $(HEADERS) Makefile
	gcc $(CFLAGS) -o $@ -c $<
