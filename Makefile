#default: optimizations enabled
OPT?=1

CFLAGS_BASE:=-m32 -march=native -mtune=native -fno-strict-aliasing -pthread -g3 -gdwarf-4 -fvar-tracking-assignments -Wall -Wno-unused-variable -Wno-unused-function -fdiagnostics-color=always -I. -Iosx

ifeq "$(OPT)" "0"
	CFLAGS_BASE+=-O0 -fno-omit-frame-pointer
else
	CFLAGS_BASE+=-O2
endif

CFLAGS  :=$(CFLAGS_BASE) -std=c17
CXXFLAGS:=$(CFLAGS_BASE) -std=c++17

LDFLAGS:=-Wl,-rpath=. -lm -ldl -lbsd -lelf -Wl,-E libiberty.a

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
