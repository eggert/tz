# Generate the 'leapseconds' file from 'leap-seconds.list'.

# This file is in the public domain.

BEGIN {
  print "# Allowance for leap seconds added to each time zone file."
  print ""
  print "# This file is in the public domain."
  print ""
  print "# This file is generated automatically from the data in the public-domain"
  print "# leap-seconds.list file, which can be copied from"
  print "# <ftp://ftp.nist.gov/pub/time/leap-seconds.list>"
  print "# or <ftp://ftp.boulder.nist.gov/pub/time/leap-seconds.list>"
  print "# or <ftp://tycho.usno.navy.mil/pub/ntp/leap-seconds.list>."
  print "# For more about leap-seconds.list, please see"
  print "# The NTP Timescale and Leap Seconds"
  print "# <https://www.eecis.udel.edu/~mills/leap.html>."
  print ""
  print "# The International Earth Rotation and Reference Systems Service"
  print "# periodically uses leap seconds to keep UTC to within 0.9 s of UT1"
  print "# (which measures the true angular orientation of the earth in space)"
  print "# and publishes leap second data in a copyrighted file"
  print "# <https://hpiers.obspm.fr/iers/bul/bulc/Leap_Second.dat>."
  print "# See: Levine J. Coordinated Universal Time and the leap second."
  print "# URSI Radio Sci Bull. 2016;89(4):30-6. doi:10.23919/URSIRSB.2016.7909995"
  print "# <https://ieeexplore.ieee.org/document/7909995>."
  print "# There were no leap seconds before 1972, because the official mechanism"
  print "# accounting for the discrepancy between atomic time and the earth's rotation"
  print "# did not exist."
  print ""
  print "# The correction (+ or -) is made at the given time, so lines"
  print "# will typically look like:"
  print "#	Leap	YEAR	MON	DAY	23:59:60	+	R/S"
  print "# or"
  print "#	Leap	YEAR	MON	DAY	23:59:59	-	R/S"
  print ""
  print "# If the leap second is Rolling (R) the given time is local time (unused here)."
  print "# If the leap second is Stationary (S) the given time is UTC."
  print ""
  print "# Leap	YEAR	MONTH	DAY	HH:MM:SS	CORR	R/S"
}

/^#\tUpdated through/ || /^#\tFile expires on:/ {
    last_lines = last_lines $0 "\n"
}

/^#[$][ \t]/ { updated = $2 }
/^#[@][ \t]/ { expires = $2 }

/^#/ { next }

{
    NTP_timestamp = $1
    TAI_minus_UTC = $2
    hash_mark = $3
    one = $4
    month = $5
    year = $6
    if (old_TAI_minus_UTC) {
	if (old_TAI_minus_UTC < TAI_minus_UTC) {
	    sign = "23:59:60\t+"
	} else {
	    sign = "23:59:59\t-"
	}
	if (month == "Jan") {
	    year--;
	    month = "Dec";
	    day = 31
	} else if (month == "Jul") {
	    month = "Jun";
	    day = 30
	}
	printf "Leap\t%s\t%s\t%s\t%s\tS\n", year, month, day, sign
    }
    old_TAI_minus_UTC = TAI_minus_UTC
}

END {
    # The difference between the NTP and POSIX epochs is 70 years
    # (including 17 leap days), each 24 hours of 60 minutes of 60
    # seconds each.
    epoch_minus_NTP = ((1970 - 1900) * 365 + 17) * 24 * 60 * 60

    print ""
    print "# POSIX timestamps for the data in this file:"
    printf "#updated %s\n", updated - epoch_minus_NTP
    printf "#expires %s\n", expires - epoch_minus_NTP
    printf "\n%s", last_lines
}
