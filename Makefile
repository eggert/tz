# %W%

# If you want something other than Eastern United States time used on your
# system, change the line below (after finding the zone you want in the
# time zone files, or adding it to a time zone file).
# Alternately, if you discover you've got the wrong time zone, you can just
#	tzcomp -l rightzone

LOCALTIME=	Eastern

# Use an absolute path name for TZDIR unless you're just testing the software.

TZDIR=		/etc/tzdir

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
DATA=		asia australasia europe etcetera northamerica pacificnew
ENCHILADA=	README Makefile settz.3 tzfile.5 tzcomp.8 \
			tzfile.h $(TZCSRCS) $(TZDSRCS) years.sh $(DATA)

all:	REDID_BINARIES tzdump

REDID_BINARIES:	$(TZDIR) tzcomp $(DATA) years
	tzcomp -l $(LOCALTIME) -d $(TZDIR) $(DATA)
	cp /dev/null $@

tzdump:	$(TZDOBJS)
	$(CC) $(LFLAGS) $(TZDOBJS) $(LIBS) -o $@

tzcomp:	$(TZCOBJS)
	$(CC) $(LFLAGS) $(TZCOBJS) $(LIBS) -o $@

$(TZDIR):
	mkdir $@

years:	years.sh
	rm -f $@
	cp $? $@
	chmod 555 $@

BUNDLE:	$(ENCHILADA)
	bundle $(ENCHILADA) > BUNDLE

$(ENCHILADA):
	sccs get $(REL) $(REV) $@

sure:	$(TZCSRCS) $(TZDSRCS)
	lint $(LINTFLAGS) $(TZCSRCS)
	lint $(LINTFLAGS) $(TZDSRCS)

clean:
	rm -f core *.o *.out REDID_BINARIES years tzdump tzcomp BUNDLE \#*

CLEAN:	clean
	sccs clean

tzdump.o tzcomp.o settz.o:	tzfile.h
