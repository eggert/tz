# %W%

# If you want something other than Eastern United States time used on your
# system, change the line below (after finding the zone you want in the
# time zone files, or adding it to a time zone file).
# Alternately, if you discover you've got the wrong time zone, you can just
#	tzcomp -l rightzone

LOCALTIME=	US/Eastern

# Use an absolute path name for TZDIR unless you're just testing the software.

TZDIR=		/etc/timezones

# You may want to change this define if you're just testing the software.

TZLIB=		/usr/lib/libtz.a

# LINTFLAGS is set for 4.1bsd systems.  If you're using System V, you'll want
# to comment out the "LINTFLAGS=" line.

LINTFLAGS=	-phbaaxc

DEBUG=
LFLAGS=
CFLAGS=		$(DEBUG) -O -DOBJECTID -DTZDIR=\"$(TZDIR)\"

TZCSRCS=	tzcomp.c scheck.c strchr.c mkdir.c
TZCOBJS=	tzcomp.o scheck.o strchr.o mkdir.o
TZDSRCS=	tzdump.c settz.c
TZDOBJS=	tzdump.o settz.o
DOCS=		README Makefile settz.3 tzfile.5 tzcomp.8
SOURCES=	tzfile.h $(TZCSRCS) $(TZDSRCS) years.sh ctime.c
DATA=		asia australasia europe etcetera northamerica pacificnew
ENCHILADA=	$(DOCS) $(SOURCES) $(DATA)

all:	REDID_BINARIES tzdump $(TZLIB)

REDID_BINARIES:	$(TZDIR) tzcomp $(DATA) years
	./tzcomp -l $(LOCALTIME) -d $(TZDIR) $(DATA)
	cp /dev/null $@

tzdump:	$(TZDOBJS)
	$(CC) $(LFLAGS) $(TZDOBJS) -o $@

$(TZLIB):	ctime.o settz.o
	ar ru $@ ctime.o settz.o
	ranlib $@

tzcomp:	$(TZCOBJS)
	$(CC) $(LFLAGS) $(TZCOBJS) -o $@

$(TZDIR):
	mkdir $@

years:	years.sh
	rm -f $@
	cp $? $@
	chmod 555 $@

BUNDLE1:	$(DOCS)
	bundle $(DOCS) > BUNDLE1

BUNDLE2:	$(SOURCES)
	bundle $(SOURCES) > BUNDLE2

BUNDLE3:	$(DATA)
	bundle $(DATA) > BUNDLE3

$(ENCHILADA):
	sccs get $(REL) $(REV) $@

sure:	$(TZCSRCS) $(TZDSRCS) tzfile.h ctime.c
	lint $(LINTFLAGS) $(TZCSRCS)
	lint $(LINTFLAGS) $(TZDSRCS)
	lint $(LINTFLAGS) ctime.c

clean:
	rm -f core *.o *.out REDID_BINARIES years tzdump tzcomp BUNDLE \#*

CLEAN:	clean
	sccs clean

tzdump.o tzcomp.o settz.o:	tzfile.h
