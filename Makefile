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
# Alternatively, you can put these functions in /lib/libc.a, removing
# the old "ctime.o" (and "timezone.o" on a BSD system).  This is the
# ideal solution if you are able.  Build libz.a, extract the files, and
# then add them to libc.a.

TZLIB=		/usr/lib/libz.a

#
# If you're running on a System V-style system and don't want lint grief,
# add
#	-DUSG
# to the end of the "CFLAGS=" line.
#
# If you're running on a system where "strchr" is known as "index",
# (for example, a 4.[012]BSD system), add
#	-Dstrchr=index
# to the end of the "CFLAGS=" line.
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
# compatibility code, add
#	-DTZA_COMPAT
# to the end of the "CFLAGS=" line.
#
# If you'd like to use Robert Elz's additions to the "struct tm" structure,
# add a
#	-DKRE_COMPAT
# to the end of the "CFLAGS=" line, and add the structure elements
#	long	tm_gmtoff;
#	char *	tm_zone;
# to the END of the "struct tm" structure defined in "/usr/include/time.h".
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
# (and remove solar87 and solar88 from the DATA= line below).
#

CFLAGS=

# LINTFLAGS is set for 4.[123]BSD systems.
# If you're using System V, you'll want to comment out the "LINTFLAGS=" line.

LINTFLAGS=	-phbaaxc

SHAR=		shar

################################################################################

CC=		cc -DTZDIR=\"$(TZDIR)\"

TZCSRCS=	zic.c localtime.c asctime.c scheck.c ialloc.c mkdir.c
TZCOBJS=	zic.o localtime.o asctime.o scheck.o ialloc.o mkdir.o
TZDSRCS=	zdump.c localtime.c asctime.c ialloc.c
TZDOBJS=	zdump.o localtime.o asctime.o ialloc.o
LIBSRCS=	localtime.c asctime.c ctime.c dysize.c timemk.c
LIBOBJS=	localtime.o asctime.o ctime.o dysize.o timemk.o
DOCS=		Patchlevel.h \
		README Theory \
		newctime.3 tzfile.5 zic.8 zdump.8 \
		Makefile
SOURCES=	tzfile.h zic.c zdump.c \
		localtime.c asctime.c ctime.c dysize.c timemk.c \
		scheck.c ialloc.c mkdir.c
DATA=		africa antarctica asia australasia \
		europe northamerica southamerica \
		pacificnew systemv solar87 solar88
XDATA=		leapseconds
ENCHILADA=	$(DOCS) $(SOURCES) $(DATA) $(XDATA)

all:		REDID_BINARIES zdump $(TZLIB)

REDID_BINARIES:	zic $(DATA)
		PATH=.:$$PATH zic -d $(TZDIR) $(DATA) && \
		PATH=.:$$PATH zic -d $(TZDIR) -L /dev/null systemv && \
		PATH=.:$$PATH zic -d $(TZDIR) -l $(LOCALTIME) && \
		> $@

zdump:		$(TZDOBJS)
		$(CC) $(CFLAGS) $(LFLAGS) $(TZDOBJS) -o $@

$(TZLIB):	$(LIBOBJS)
		ar ru $@ $(LIBOBJS)
		test -f /usr/bin/ranlib && ranlib $@

zic:		$(TZCOBJS)
		$(CC) $(CFLAGS) $(LFLAGS) $(TZCOBJS) -o $@

SHARS:		SHAR1 SHAR2 SHAR3

SHAR1:		$(DOCS)
		$(SHAR) $(DOCS) > $@

SHAR2:		$(SOURCES)
		$(SHAR) $(SOURCES) > $@

SHAR3:		$(DATA) $(XDATA)
		$(SHAR) $(DATA) $(XDATA) > $@

tz.shar.Z.uue:	$(ENCHILADA)
		$(SHAR) $(ENCHILADA) | compress | uuencode tz.shar.Z > $@

$(ENCHILADA):
		sccs get $(REL) $(REV) $@

sure:		$(TZCSRCS) $(TZDSRCS) tzfile.h
		lint $(LINTFLAGS) $(CFLAGS) -DTZDIR=\"$(TZDIR)\" $(TZCSRCS)
		lint $(LINTFLAGS) $(CFLAGS) -DTZDIR=\"$(TZDIR)\" $(TZDSRCS)
		lint $(LINTFLAGS) $(CFLAGS) -DTZDIR=\"$(TZDIR)\" $(LIBSRCS)

clean:
		rm -f core *.o *.out REDID_BINARIES zdump zic \
		SHAR* tz.shar.Z.uue ,*

CLEAN:		clean
		sccs clean

names:
		@echo $(ENCHILADA)

zdump.o zic.o newctime.o:	tzfile.h
