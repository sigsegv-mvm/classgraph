CFLAGS  :=-m32 -I. -I/usr/include/libiberty -std=gnu11 -Og -g -Wall -fdiagnostics-color=always -Wno-unused-variable -Wno-unused-function -march=native -mtune=native -pthread
CXXFLAGS:=-m32 -I. -I/usr/include/libiberty -std=gnu++14 -Og -g -Wall -fdiagnostics-color=always -Wno-unused-variable -Wno-unused-function -march=native -mtune=native -pthread
LDFLAGS :=-Wl,-rpath=. -lm -ldl -lbsd -lelf -Wl,-E libiberty.a

SOURCES_C:=$(shell find . -follow -type f -name '*.c')
SOURCES_CXX:=$(shell find . -follow -type f -name '*.cc')
OBJECTS:=$(patsubst %.cc,%.o,$(SOURCES_CXX)) $(patsubst %.c,%.o,$(SOURCES_C))
HEADERS:=$(shell find . -follow -type f -name '*.h')

PCH:=stdc++.h

.PHONY: all clean
all: classgraph
clean:
	rm -f classgraph $(shell find . -iname '*.o')

classgraph: $(OBJECTS) Makefile
	g++ $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

%.o: %.cc $(HEADERS) $(PCH).gch Makefile
	g++ $(CXXFLAGS) -include $(PCH) -o $@ -c $<

%.o: %.c $(HEADERS) Makefile
	gcc $(CFLAGS) -o $@ -c $<

$(PCH).gch: /usr/include/c++/5.2.0/x86_64-unknown-linux-gnu/32/bits/$(PCH) Makefile
	g++ $(CXXFLAGS) -x c++-header -o $@ $<
