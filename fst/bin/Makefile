# Note binaries need to be linked dynamically to FST libs if Fst or
# Arc type DSO extensions are to be used (see e.g. test/dso_test.sh).

LIBDIR=$(PWD)/../lib
BINDIR=$(PWD)
INCS=$(LIBDIR)/*.h main.h
BINS=fstarcsort fstclosure fstcompile fstcompose fstconcat fstconnect \
fstconvert fstdeterminize fstdifference fstdraw fstencode fstepsnormalize \
fstequal fstequivalent fstinfo fstintersect fstinvert fstmap fstminimize \
fstprint fstproject fstprune fstpush fstrandgen fstrelabel fstreplace \
fstreverse fstreweight fstrmepsilon fstshortestdistance fstshortestpath \
fstsynchronize fsttopsort fstunion
LIBTYPE=so	# "dylib" on macosx
FSTLIB=$(LIBDIR)/libfst.$(LIBTYPE)
FSTLIBMAIN=libfstmain.$(LIBTYPE)
LIBS=-lfstmain -lfst -lm -lpthread -ldl
CC=g++
OPT=-O2
CFLAGS=-I../.. $(OPT) -DFST_DL
LDLFLAGS=-Wl,-L$(LIBDIR),-L$(BINDIR)
LDRFLAGS=-Wl,-rpath,$(LIBDIR),-rpath,$(BINDIR) # empty on macosx
LDFLAGS=$(LDLFLAGS) $(LDRFLAGS)
SOFLAGS=-shared  # "-dynamiclib -flat_namespace -undefined suppress" on macosx

all: $(BINS)

fstarcsort: fstarcsort.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstarcsort.o: fstarcsort.cc arcsort-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstclosure: fstclosure.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstclosure.o: fstclosure.cc closure-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstcompile: fstcompile.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstcompile.o: fstcompile.cc compile-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstcompose: fstcompose.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstcompose.o: fstcompose.cc compose-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstconcat: fstconcat.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstconcat.o: fstconcat.cc concat-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstconnect: fstconnect.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstconnect.o: fstconnect.cc connect-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstconvert: fstconvert.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstconvert.o: fstconvert.cc convert-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstdraw: fstdraw.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstdraw.o: fstdraw.cc draw-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstdeterminize: fstdeterminize.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstdeterminize.o: fstdeterminize.cc determinize-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstdifference: fstdifference.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstdifference.o: fstdifference.cc difference-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstencode: fstencode.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstencode.o: fstencode.cc encode-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstepsnormalize: fstepsnormalize.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstepsnormalize.o: fstepsnormalize.cc epsnormalize-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstequal: fstequal.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstequal.o: fstequal.cc equal-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstequivalent: fstequivalent.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstequivalent.o: fstequivalent.cc equivalent-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstinfo: fstinfo.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstinfo.o: fstinfo.cc info-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstintersect: fstintersect.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstintersect.o: fstintersect.cc intersect-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstinvert: fstinvert.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstinvert.o: fstinvert.cc invert-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstmap: fstmap.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstmap.o: fstmap.cc map-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstminimize: fstminimize.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstminimize.o: fstminimize.cc minimize-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstprint: fstprint.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstprint.o: fstprint.cc print-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstproject: fstproject.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstproject.o: fstproject.cc project-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstprune: fstprune.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstprune.o: fstprune.cc prune-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstpush: fstpush.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstpush.o: fstpush.cc push-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstrandgen: fstrandgen.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstrandgen.o: fstrandgen.cc randgen-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstrelabel: fstrelabel.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstrelabel.o: fstrelabel.cc relabel-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstreplace: fstreplace.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstreplace.o: fstreplace.cc replace-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstreverse: fstreverse.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstreverse.o: fstreverse.cc reverse-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstreweight: fstreweight.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstreweight.o: fstreweight.cc reweight-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstrmepsilon: fstrmepsilon.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstrmepsilon.o: fstrmepsilon.cc rmepsilon-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstshortestdistance: fstshortestdistance.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstshortestdistance.o: fstshortestdistance.cc shortest-distance-main.h \
  $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstshortestpath: fstshortestpath.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstshortestpath.o: fstshortestpath.cc shortest-path-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstsynchronize: fstsynchronize.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstsynchronize.o: fstsynchronize.cc synchronize-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fsttopsort: fsttopsort.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fsttopsort.o: fsttopsort.cc topsort-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

fstunion: fstunion.o $(FSTLIB) $(FSTLIBMAIN)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIBS)

fstunion.o: fstunion.cc union-main.h $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

libfstmain.a: main.o $(FSTLIB)
	rm -f $@
	ar rc $@ main.o

libfstmain.$(LIBTYPE): main.o $(FSTLIB)
	$(CC) $(SOFLAGS) -o $@ main.o

main.o: main.cc $(INCS) Makefile
	$(CC) $(CFLAGS) -o $@ -c $*.cc

clean:
	rm -f $(FSTLIB) $(FSTLIBMAIN) *.o $(BINS) *~
