# %W%

LINTFLAGS=	-phbaaxc

CFLAGS=		-g -DOBJECTID 

TZCFLAGS=	-d tzdir	# Remove this line to put files in /etc/tzdir
TZCSRCS=	tzcomp.c scheck.c strchr.c
TZCOBJS=	tzcomp.o scheck.o strchr.o

ENCHILADA=	Makefile timezone.h settz.c tzdump.c $(TZCSRCS) tzinfo types.sh

ALL=		tzdump tzcomp uspres nonpres

all:	$(ALL)

data:	tzcomp tzinfo
	tzcomp $(TZCFLAGS) tzinfo

uspres nonpres:	types.sh
	cp $? $@
	chmod +x $@

tzdump:	tzdump.o settz.o
		$(CC) $(CFLAGS) tzdump.o settz.o $(LIBS) -o $@

tzcomp:	$(TZCOBJS)
		$(CC) $(CFLAGS) $(TZCOBJS) $(LIBS) -o $@

bundle:	$(ENCHILADA)
	bundle $(ENCHILADA) > bundle

$(ENCHILADA):
		sccs get $(REL) $(REV) $@

sure:	tzdump.c settz.c
		lint $(LINTFLAGS) tzdump.c settz.c
		lint $(LINTFLAGS) $(TZCSRCS)

clean:
		rm -f core *.o *.out $(ALL) bundle \#*

CLEAN:	clean
		sccs clean

tzdump.o tzcomp.o settz.o:	timezone.h
