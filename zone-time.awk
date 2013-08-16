# Generate 'time.tab' from 'zone.tab'.  Standard input should be the zic input.

# This file is in the public domain.

# Contributed by Paul Eggert.

$1 == "Link" { link[$3] = $2 }

END {
    FS = "\t"
    while (getline < "zone.tab") {
	line = $0
	if (line ~ /^# TZ zone descriptions/)
	    line = "# TZ zone descriptions, with a smaller set of zone names"
	if (line ~ /^# 4.  Comments;/) {
	    print "#     Zones can cross country-code boundaries, so the"
	    print "#     location named by column 3 need not lie in the"
	    print "#     locations identified by columns 1 or 2."
	}
	if (line ~ /^[^#]/) {
	    code = $1
	    target = $3
	    while (link[target])
		target = link[target]
	    if (already_seen[code, target])
		continue
	    already_seen[code, target] = 1
	    line = code "\t" $2 "\t" target
	    if ($4)
		line = line "\t" $4
	}
	print line
    }
}
