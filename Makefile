# %W%

# Use an absolute path name for TZDIR unless you're just testing the software.

TZDIR=		/usr/local/lib/tzdir
DEBUG=
LINTFLAGS=	-phbaaxc
LFLAGS=
CFLAGS=		$(DEBUG) -O -DOBJECTID -DTZDIR=\"$(TZDIR)\"

TZCSRCS=	tzcomp.c scheck.c strchr.c
TZCOBJS=	tzcomp.o scheck.o strchr.o
ENCHILADA=	Makefile timezone.h settz.c tzdump.c $(TZCSRCS) tzinfo years.sh

all:	REDID_BINARIES tzdump

REDID_BINARIES:	$(TZDIR) tzcomp tzinfo years
	tzcomp -d $(TZDIR) tzinfo
	cp /dev/null $@

tzdump:	tzdump.o settz.o
	$(CC) $(LFLAGS) tzdump.o settz.o $(LIBS) -o $@

tzcomp:	$(TZCOBJS)
	$(CC) $(LFLAGS) $(TZCOBJS) $(LIBS) -o $@

$(TZDIR):
	mkdir $@

years:	years.sh
	rm -f $@
	cp $? $@
	chmod 555 $@

bundle:	$(ENCHILADA)
	bundle $(ENCHILADA) > bundle

$(ENCHILADA):
	sccs get $(REL) $(REV) $@

sure:	tzdump.c $(TZCSRCS)
	lint $(LINTFLAGS) tzdump.c settz.c
	lint $(LINTFLAGS) $(TZCSRCS)

clean:
	rm -f core *.o *.out REDID_BINARIES years tzdump tzcomp bundle \#*

CLEAN:	clean
	sccs clean

tzdump.o tzcomp.o settz.o:	timezone.h
