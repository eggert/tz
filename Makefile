# %W%

# If you want something other than Eastern United States time used on your
# system, change the line below (after finding the zone you want in the
# time zone files, or adding it to a time zone file).
# Alternately, if you discover you've got the wrong time zone, you can just
#	zic -l rightzone

LOCALTIME=	US/Eastern

# Use an absolute path name for TZDIR unless you're just testing the software.

TZDIR=		/etc/zoneinfo

# You may want to change this define if you're just testing the software.

TZLIB=		/usr/lib/libtz.a

# LINTFLAGS is set for 4.1bsd systems.  If you're using System V, you'll want
# to comment out the "LINTFLAGS=" line.

LINTFLAGS=	-phbaaxc

LFLAGS=
CFLAGS=		-g -DOBJECTID -DTZDIR=\"$(TZDIR)\"

TZCSRCS=	zic.c scheck.c strchr.c mkdir.c
TZCOBJS=	zic.o scheck.o strchr.o mkdir.o
TZDSRCS=	zdump.c newctime.c
TZDOBJS=	zdump.o newctime.o
DOCS=		README Makefile newctime.3 tzfile.5 zic.8 zdump.8
SOURCES=	tzfile.h $(TZCSRCS) $(TZDSRCS)
DATA=		asia australasia europe etcetera northamerica pacificnew systemv
ENCHILADA=	$(DOCS) $(SOURCES) $(DATA)

all:	REDID_BINARIES zdump $(TZLIB)

REDID_BINARIES:	$(TZDIR) zic $(DATA)
	PATH=.:$$PATH zic -l $(LOCALTIME) -d $(TZDIR) $(DATA) && > $@

zdump:	$(TZDOBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $(TZDOBJS) -o $@

$(TZLIB):	newctime.o
	ar ru $@ newctime.o
	ranlib $@

zic:	$(TZCOBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $(TZCOBJS) -o $@

$(TZDIR):
	mkdir $@

BUNDLE1:	$(DOCS)
	bundle $(DOCS) > BUNDLE1

BUNDLE2:	$(SOURCES)
	bundle $(SOURCES) > BUNDLE2

BUNDLE3:	$(DATA)
	bundle $(DATA) > BUNDLE3

$(ENCHILADA):
	sccs get $(REL) $(REV) $@

sure:	$(TZCSRCS) $(TZDSRCS) tzfile.h
	lint $(LINTFLAGS) $(TZCSRCS)
	lint $(LINTFLAGS) $(TZDSRCS)

clean:
	rm -f core *.o *.out REDID_BINARIES zdump zic BUNDLE \#*

CLEAN:	clean
	sccs clean

zdump.o zic.o newctime.o:	tzfile.h
