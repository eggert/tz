#! /bin/sh

# %W%

echo ".hy 0
.na
.de }H
..
.de }F
.." | nroff -man - ${1+"$@"} | perl -ne '
	chomp;
	s/.\010//g;
	s/\s*$//;
	if (/^$/) {
		$sawblank = 1;
		next;
	} else {
		if ($sawblank && $didprint) {
			print "\n";
			$sawblank = 0;
		}
		print "$_\n";
		$didprint = 1;
	}
' 
