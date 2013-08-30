# Generate 'back-pre1970' from the two input files 'pre1970' and 'backward'.
# The output consists of all lines in 'backward' that are not links to
# files mentioned in 'pre1970'.  Think of it as 'backward' minus 'pre1970'.

# The 'backward' file is the input.
# The awk variable 'pre1970' contains the name of the pre1970 file.

# This file is in the public domain.

# Contributed by Paul Eggert.

BEGIN {
    while ((getline <pre1970) == 1)
	if ($1 == "Zone")
	    pre1970_zone[$2] = 1
}

! (/^Link/ && pre1970_zone[$3]) { print }
