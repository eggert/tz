# %W%

# If you want something other than Eastern United States time used on your
# system, change the line below (after finding the zone you want in tzinfo,
# or adding it to tzinfo).  Alternately, if you discover you've got the
# wrong time zone, you can just
#	tzcomp -l rightzone

LOCALTIME=	Eastern

# Since this stuff isn't part of any official release, we'll put it in the
# directory /usr/local/lib/tzdir rather than the directory /etc/tzdir that's
# mentioned in the writeups.  You may want to change that.  In any case,
# use an absolute path name for TZDIR unless you're just testing the software.

TZDIR=		/usr/local/lib/tzdir

DEBUG=
LINTFLAGS=	-phbaaxc
LFLAGS=
CFLAGS=		$(DEBUG) -O -DOBJECTID -DTZDIR=\"$(TZDIR)\"

TZCSRCS=	tzcomp.c scheck.c strchr.c
TZCOBJS=	tzcomp.o scheck.o strchr.o
TZDSRCS=	tzdump.c settz.c
TZDOBJS=	tzdump.o settz.o
ENCHILADA=	Makefile tzfile.h $(TZCSRCS) $(TZDSRCS) tzinfo years.sh \
			README settz.3 tzfile.5 tzcomp.8

all:	REDID_BINARIES tzdump

REDID_BINARIES:	$(TZDIR) tzcomp tzinfo years
	tzcomp -l $(LOCALTIME) -d $(TZDIR) tzinfo
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

bundle:	$(ENCHILADA)
	bundle $(ENCHILADA) > bundle

$(ENCHILADA):
	sccs get $(REL) $(REV) $@

sure:	$(TZCSRCS) $(TZDSRCS)
	lint $(LINTFLAGS) $(TZCSRCS)
	lint $(LINTFLAGS) $(TZDSRCS)

clean:
	rm -f core *.o *.out REDID_BINARIES years tzdump tzcomp bundle \#*

CLEAN:	clean
	sccs clean

tzdump.o tzcomp.o settz.o:	tzfile.h
