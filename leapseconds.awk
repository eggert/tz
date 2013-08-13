# Generate the 'leapseconds' file from 'leap-seconds.list'.

# This file is in the public domain.

BEGIN {
    printf "%s", "\
# Allowance for leapseconds added to each timezone file.\n\
\n\
# This file is in the public domain.\n\
\n\
# This file is generated automatically from the data in the public-domain\n\
# leap-seconds.list file available from most NIST time servers.\n\
# If the URL <ftp://time.nist.gov/pub/leap-seconds.list> does not work,\n\
# you should be able to pick up leap-seconds.list from a secondary NIST server.\n\
# For more about leap-seconds.list, please see\n\
# The NTP Timescale and Leap Seconds\n\
# <http://www.eecis.udel.edu/~mills/leap.html>.\n\
\n\
# The International Earth Rotation Service periodically uses leap seconds\n\
# to keep UTC to within 0.9 s of UT1\n\
# (which measures the true angular orientation of the earth in space); see\n\
# Terry J Quinn, The BIPM and the accurate measure of time,\n\
# Proc IEEE 79, 7 (July 1991), 894-905 <http://dx.doi.org/10.1109/5.84965>.\n\
# There were no leap seconds before 1972, because the official mechanism\n\
# accounting for the discrepancy between atomic time and the earth's rotation\n\
# did not exist until the early 1970s.\n\
\n\
# The correction (+ or -) is made at the given time, so lines\n\
# will typically look like:\n\
#	Leap	YEAR	MON	DAY	23:59:60	+	R/S\n\
# or\n\
#	Leap	YEAR	MON	DAY	23:59:59	-	R/S\n\
\n\
# If the leapsecond is Rolling (R) the given time is local time.\n\
# If the leapsecond is Stationary (S) the given time is UTC.\n\
\n\
# Leap	YEAR	MONTH	DAY	HH:MM:SS	CORR	R/S\n\
"
}

/^ *$/ { next }
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
