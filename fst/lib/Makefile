INCS=*.h
SRCS=fst.cc properties.cc symbol-table.cc compat.cc
OBJS=fst.o properties.o symbol-table.o compat.o
CC=g++
OPT=-O2
CFLAGS=$(OPT) -I../.. -DFST_DL
LIBTYPE=so # "dylib" on macosx
SOFLAGS=-shared  # "-dynamiclib -flat_namespace -undefined suppress" on macosx

all: libfst.a libfst.$(LIBTYPE)

libfst.a: $(OBJS)
	rm -f $@
	ar rc $@ $(OBJS)

libfst.$(LIBTYPE): $(OBJS)
	$(CC) $(SOFLAGS) -o $@ $(OBJS)

$(OBJS): $(INCS) $(SRCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

clean:
	rm -f libfst.* *.o *.a *.so *~
