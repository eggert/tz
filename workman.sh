#! /bin/sh

# %W%

echo ".hy 0
.pl 99i
.na" |
	nroff -man - ${1+"$@"} |
	perl -ne '
		chomp;
		s/.\010//g;
		s/[ 	]*$//;
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
