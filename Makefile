# %W%

LINTFLAGS=	-phbaaxc

CFLAGS=		-g -DOBJECTID 

TZCSRCS=	tzcomp.c scheck.c strchr.c
TZCOBJS=	tzcomp.o scheck.o strchr.o

WHOLE_ENCHILADA=	Makefile timezone.h settz.c try.c $(TZCSRCS) tzinfo

ALL=		try tzcomp

all:	$(ALL)

data:
	tzcomp tzinfo

try:	try.o settz.o
		$(CC) $(CFLAGS) try.o settz.o $(LIBS) -o $@

tzcomp:	$(TZCOBJS)
		$(CC) $(CFLAGS) $(TZCOBJS) $(LIBS) -o $@

bundle:	$(WHOLE_ENCHILADA)
	bundle $(WHOLE_ENCHILADA) > bundle

$(WHOLE_ENCHILADA):
		sccs get $(REL) $(REV) $@

sure:	try.c settz.c fake.c
		lint $(LINTFLAGS) try.c settz.c
		lint $(LINTFLAGS) $(TZCSRCS)

clean:
		rm -f core *.o *.out $(ALL) bundle \#*

CLEAN:	clean
		sccs clean

try.o tzcomp.o settz.o:	timezone.h
