LIBDIR=$(PWD)/../lib
BINDIR=$(PWD)/../bin
INCS=$(LIBDIR)/*.h $(BINDIR)/*.h
LIBTYPE=so	# "dylib" on macosx
BINS= algo_test fst_test weight_test
SOS=basic-fst.so pair-arc.so
FSTLIB=$(LIBDIR)/libfst.$(LIBTYPE)
LIBS=-lfst -lm -lpthread -ldl
CC=g++
OPT=-O2
CFLAGS=-I../.. $(OPT) -DFST_DL
LDLFLAGS=-Wl,-L$(LIBDIR)
LDRFLAGS=-Wl,-rpath,$(LIBDIR) # empty on macosx
LDFLAGS=$(LDLFLAGS) $(LDRFLAGS)
SOFLAGS=-shared  # "-dynamiclib -flat_namespace -undefined suppress" on macosx
DSOFLAGS=-shared # "-bundle -flat_namespace -undefined suppress" on macosx

all: $(BINS) $(SOS)

algo_test: algo_test.o $(FSTLIB)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

algo_test.o: algo_test.cc $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fst_test: fst_test.o $(FSTLIB)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fst_test.o: fst_test.cc $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

basic-fst.so: basic-fst.o
	$(CC) $(DSOFLAGS) -o basic-fst.so basic-fst.o

basic-fst.o: basic-fst.cc $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

pair-arc.so: pair-arc.o
	$(CC) $(DSOFLAGS) -o pair-arc.so pair-arc.o

pair-arc.o: pair-arc.cc $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

weight_test: weight_test.o $(FSTLIB)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

weight_test.o: weight_test.cc $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

test_algos: algo_test
	./algo_test -repeat=5  # Tests Fst algorithms

test_fsts: fst_test
	./fst_test             # Tests Fst classes

test_weights: weight_test
	./weight_test          # Tests Weight classes

test_dsos: dso_test.sh basic-fst.so pair-arc.so
	./dso_test.sh          # Tests DSO extensions

test: test_weights test_fsts test_algos test_dsos

clean:
	rm -f $(FSTLIB) *.o *.so $(BINS) *~
