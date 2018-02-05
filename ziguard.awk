# Convert tzdata source into vanguard or rearguard form.

# Contributed by Paul Eggert.  This file is in the public domain.

# This is not a general-purpose converter; it is designed for current tzdata.
#
# When converting to vanguard form, the output can use fractional seconds
# and negative DST offsets.
#
# When converting to rearguard form, the output omits fractional
# seconds and uses only positive DST offsets.  The idea is for the
# output data to simulate the behavior of the input data as best it
# can within the constraints of the rearguard format.

BEGIN {
  dst_type["vanguard.zi"] = 1
  dst_type["main.zi"] = 1
  dst_type["rearguard.zi"] = 1

  # The command line should set OUTFILE to the name of the output file.
  if (!dst_type[outfile]) exit 1
  vanguard = outfile == "vanguard.zi"

  # List non-integer standard times more accurately if known.
  # This list does not attempt to record every UT offset that is
  # not an integral multiple of 1 s; it merely records those that
  # do not appear to be just LMT.
  frac["-5:36:13"] = "-5:36:13.3" # America/Costa_Rica before 1921
  frac["-5:07:10"] = "-5:07:10.41" # America/Jamaica before 1912
  frac["-4:16:48"] = "-4:16:48.25" # America/Cordoba etc. 1894-1920
  frac["-0:36:45"] = "-0:36:44.68" # Europe/Lisbon before 1912
  frac["-0:25:21"] = "-0:25:21.1" # Europe/Dublin 1880-1916
  frac["0:19:32"] = "0:19:32.13" # Europe/Amsterdam before 1937
  frac["1:39:49"] = "1:39:49.2" # Europe/Helsinki before 1921
  frac["2:05:09"] = "2:05:08.9" # Africa/Cairo before 1900
  frac["4:37:11"] = "4:37:10.8" # Asia/Tashkent before 1924
  frac["7:06:30"] = "7:06:30.1333" #... Asia/Ho_Chi_Minh 1906-1911
  frac["7:07:12"] = "7:07:12.5" # Asia/Jakarta before 1923
  frac["7:36:42"] = "7:36:41.7" # Asia/Hong_Kong before 1904
  frac["8:05:43"] = "8:05:43.2" # Asia/Shanghai before 1901

  fract["23:47:12"] = "23:47:12.5" # Asia/Jakarta 1923-12-31 transition
}

/^Zone/ { zone = $2 }

outfile != "main.zi" {
  in_comment = /^#/

  # If this line should differ due to Ireland using negative DST offsets,
  # uncomment the desired version and comment out the undesired one.
  Rule_Eire = /^#?Rule[\t ]+Eire[\t ]/
  Zone_Dublin_post_1968 \
    = (zone == "Europe/Dublin" && /^#?[\t ]+[01]:00[\t ]/ \
       && (!$(in_comment + 4) || 1968 < $(in_comment + 4)))
  if (Rule_Eire || Zone_Dublin_post_1968) {
    if ((Rule_Eire \
	 || (Zone_Dublin_post_1968 && $(in_comment + 3) == "IST/GMT"))	\
	== vanguard) {
      sub(/^#/, "")
    } else if (/^[^#]/) {
      sub(/^/, "#")
    }
  }

  # Add or remove fractional seconds as needed in UT offsets.
  f = $1 == "Zone" ? 3 : 1
  for (rounded in frac) {
    original = frac[rounded]
    if ($f == rounded || $f == original) {
      $f = vanguard ? original : rounded
    }
  }
  # Likewise for transition times.
  for (rounded in fract) {
    original = fract[rounded]
    if ($(f + 6) == rounded || $(f + 6) == original) {
      $(f + 6) = vanguard ? original : rounded
    }
  }
}

# If a Link line is followed by a Zone line for the same data, comment
# out the Link line.  This can happen if backzone overrides a Link
# with a Zone.
/^Link/ {
  linkline[$3] = NR
}
/^Zone/ {
  sub(/^Link/, "#Link", line[linkline[$2]])
}

{ line[NR] = $0 }

END {
  for (i = 1; i <= NR; i++)
    print line[i]
}
