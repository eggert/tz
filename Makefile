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

TZLIB=		/usr/lib/libz.a

#
# If you're running on a System V-style system and don't want lint grief,
# add
#	-DUSG
# to the end of the "CFLAGS=" line.
#
# If you're running on a system where "strchr" is known as "index",
# for example, a 4.[012]BSD system), add
#	-Dstrchr=index
# to the end of the  "CFLAGS=" line.
#
# If you want to use System V compatibility code, add
#	-DUSG_COMPAT
# to the end of the "CFLAGS=" line.
#
# If you want BSD compatibility code, add
#	-DBSD_COMPAT
# to the end of the "CFLAGS=" line.
#
# If you've used older versions of this software and want "tz_abbr"
# compatibility  code, add
#	-DTZA_COMPAT
# to the end of the "CFLAGS=" line.
#
# If you'd like to use Robert Elz's additions to the "struct tm" structure,
# add a
#	-DKRE_COMPAT
# to the end of the "CFLAGS=" line.
#
# If you want code inspired by certain emerging standards, add
#	-DSTD_INSPIRED
# to the end of the "CFLAGS=" line.
#
# If you want Source Code Control System ID's left out of object modules, add
#	-DNOID
# to the end of the "CFLAGS=" line.
#
# If you'll never want to handle solar-time-based time zones, add
#	-DNOSOLAR
# to the end of the "CFLAGS=" line
# (and remove solar87 from the SOURCES= line below).
#

CFLAGS=

# LINTFLAGS is set for 4.[123]BSD systems.
# If you're using System V, you'll want to comment out the "LINTFLAGS=" line.

LINTFLAGS=	-phbaaxc

################################################################################

CC=		cc -DTZDIR=\"$(TZDIR)\"

TZCSRCS=	zic.c scheck.c ialloc.c mkdir.c
TZCOBJS=	zic.o scheck.o ialloc.o mkdir.o
TZDSRCS=	zdump.c localtime.c gmtime.c asctime.c ctime.c ialloc.c
TZDOBJS=	zdump.o localtime.o gmtime.o asctime.o ctime.o ialloc.o
LIBSRCS=	localtime.c gmtime.c asctime.c ctime.c dysize.c mktime.c
LIBOBJS=	localtime.o gmtime.o asctime.o ctime.o dysize.o mktime.o
DOCS=		README Makefile newctime.3 tzfile.5 zic.8 zdump.8
SOURCES=	tzfile.h zic.c zdump.c \
		localtime.c gmtime.c asctime.c ctime.c dysize.c mktime.c \
		scheck.c ialloc.c mkdir.c
DATA=		asia australasia europe etcetera northamerica \
		pacificnew systemv solar87
ENCHILADA=	$(DOCS) $(SOURCES) $(DATA)

all:		REDID_BINARIES zdump $(TZLIB)

REDID_BINARIES:	$(TZDIR) zic $(DATA)
		PATH=.:$$PATH zic -l $(LOCALTIME) -d $(TZDIR) $(DATA) && > $@

zdump:		$(TZDOBJS)
		$(CC) $(CFLAGS) $(LFLAGS) $(TZDOBJS) -o $@

$(TZLIB):	$(LIBOBJS)
		ar ru $@ $(LIBOBJS)
		test -f /usr/bin/ranlib && ranlib $@

zic:		$(TZCOBJS)
		$(CC) $(CFLAGS) $(LFLAGS) $(TZCOBJS) -o $@

$(TZDIR):
		mkdir $@

BUNDLES:	BUNDLE1 BUNDLE2 BUNDLE3

BUNDLE1:	$(DOCS)
		bundle $(DOCS) > $@

BUNDLE2:	$(SOURCES)
		bundle $(SOURCES) > $@

BUNDLE3:	$(DATA)
		bundle $(DATA) > $@

$(ENCHILADA):
		sccs get $(REL) $(REV) $@

sure:		$(TZCSRCS) $(TZDSRCS) tzfile.h
		lint $(LINTFLAGS) $(CFLAGS) $(TZCSRCS)
		lint $(LINTFLAGS) $(CFLAGS) $(TZDSRCS)
		lint $(LINTFLAGS) $(CFLAGS) $(LIBSRCS)

clean:
		rm -f core *.o *.out REDID_BINARIES zdump zic BUNDLE* \#*

CLEAN:		clean
		sccs clean

listing:	$(ENCHILADA)
		pr $(ENCHILADA) | lpr

zdump.o zic.o newctime.o:	tzfile.h
