# %W%

FAKENAME=	Eastern
FAKEOFF=	-18000
FAKEST=		"EST"
FAKEDT=		"EDT"

LINTFLAGS=	-phbaaxc

CFLAGS=		-g -DFAKENAME=$(FAKENAME) -DFAKEOFF=$(FAKEOFF) \
			-DFAKEST='$(FAKEST)' -DFAKEDT='$(FAKEDT)'

ALL=		try tzdir/$(FAKENAME) fake

all:		$(ALL)

try:		try.o ctime.o
			$(CC) $(CFLAGS) try.o ctime.o $(LIBS) -o $@

tzdir/Eastern:	tzdir fake
		fake > $@

tzdir:
		mkdir tzdir

fake:		fake.o
			$(CC) $(CFLAGS) fake.o $(LIBS) -o $@

bundle:		Makefile timezone.h ctime.c try.c fake.c
			bundle Makefile timezone.h ctime.c try.c fake.c > bundle

Makefile timezone.h ctime.c try.c fake.c:
			sccs get $(REL) $(REV) $@

sure:		try.c ctime.c fake.c
			lint $(LINTFLAGS) try.c ctime.c
			lint $(LINTFLAGS) fake.c

clean:
			rm -f core *.o *.out $(ALL) bundle \#*

CLEAN:		clean
			sccs clean

try.o fake.o ctime.o:	timezone.h
