# Convert tzdata source into full or positive-DST form

# Contributed by Paul Eggert.  This file is in the public domain.

# This is not a general-purpose converter; it is designed for current tzdata.
#
# When converting to full form, the output can use negative DST offsets.
#
# When converting to positive-DST form, the output uses only positive
# DST offsets.  The idea is for the output data to simulate the
# behavior of the input data as best it can within the constraints of
# positive DST offsets.
#
# In the input, lines requiring the full format are commented #[full]
# and the positive DST near-equivalents are commented #[pdst].

BEGIN {
  dst_type["full"] = 1
  dst_type["pdst"] = 1

  # The command line should set OUTFILE to the name of the output file,
  # which should start with either "full" or "pdst".
  todst = substr(outfile, 1, 4)
  if (!dst_type[todst]) exit 1
}

/^Zone/ { zone = $2 }

{
  in_comment = /^#/

  # Test whether this line should differ between the full and the pdst versions.
  Rule_Eire = /^#?Rule[\t ]+Eire[\t ]/
  Zone_Dublin_post_1968 \
    = (zone == "Europe/Dublin" && /^#?[\t ]+[01]:00[\t ]/ \
       && (!$(in_comment + 4) || 1968 < $(in_comment + 4)))

  # If so, uncomment the desired version and comment out the undesired one.
  if (Rule_Eire || Zone_Dublin_post_1968) {
    if ((Rule_Eire \
	 || (Zone_Dublin_post_1968 && $(in_comment + 3) == "IST/GMT"))	\
	== (todst == "full")) {
      sub(/^#/, "")
    } else if (/^[^#]/) {
      sub(/^/, "#")
    }
  }
}

{ print }
