# Convert tzdata source into a smaller version of itself.

# Contributed by Paul Eggert.  This file is in the public domain.

# This is not a general-purpose converter; it is designed for current tzdata.
# 'zic' should treat this script's output as if it were identical to
# this script's input.

BEGIN {
  print "# This zic input file is in the public domain."
  if (PACKRATDATA) {
    while (0 < (getline line <PACKRATDATA)) {
      if (split(line, field)) {
	if (field[1] == "Zone") packrat_zone[field[2]] = 1
	if (field[1] == "Link") packrat_zone[field[3]] = 1
      }
    }
    close (PACKRATDATA)
  }
}

# Remove comments, normalize spaces, and append a space to each line.
/^[[:space:]]*[^#[:space:]]/ {
  line = $0
  sub(/#.*/, "", line)
  line = line " "
  gsub(/[[:space:]]+/, " ", line)

  # SystemV rules are not needed.
  if (line ~ /^Rule SystemV /) next

  # Replace FooAsia rules with the same rules without "Asia", as they
  # are duplicates.
  if (n = match(line, /[^ ]Asia /)) {
    if (line ~ /^Rule /) next
    line = substr(line, 1, n) substr(line, n + 5)
  }

  # Abbreviate times.
  while (n = match(line, /[: ]0+[0-9]/)) {
    line = substr(line, 1, n) substr(line, n + RLENGTH - 1)
  }
  while (n = match(line, /:0[^:]/)) {
    line = substr(line, 1, n - 1) substr(line, n + 2)
  }

  # Abbreviate weekday names.  Do not abbreviate "Sun" and "Sat", as
  # pre-2017c zic erroneously diagnoses "Su" and "Sa" as ambiguous.
  while (n = match(line, /[ l]Mon[<>]/)) {
    line = substr(line, 1, n + 1) substr(line, n + 4)
  }
  while (n = match(line, /[ l]Tue[<>]/)) {
    line = substr(line, 1, n + 2) substr(line, n + 4)
  }
  while (n = match(line, /[ l]Wed[<>]/)) {
    line = substr(line, 1, n + 1) substr(line, n + 4)
  }
  while (n = match(line, /[ l]Thu[<>]/)) {
    line = substr(line, 1, n + 2) substr(line, n + 4)
  }
  while (n = match(line, /[ l]Fri[<>]/)) {
    line = substr(line, 1, n + 1) substr(line, n + 4)
  }

  # Abbreviate "only" and month names.
  gsub(/ only /, " o ", line)
  gsub(/ Jan /, " Ja ", line)
  gsub(/ Feb /, " F ", line)
  gsub(/ Apr /, " Ap ", line)
  gsub(/ Aug /, " Au ", line)
  gsub(/ Sep /, " S ", line)
  gsub(/ Oct /, " O ", line)
  gsub(/ Nov /, " N ", line)
  gsub(/ Dec /, " D ", line)

  # Strip leading and trailing space.
  sub(/^ /, "", line)
  sub(/ $/, "", line)

  # Remove unnecessary trailing zero fields.
  sub(/ 0+$/, "", line)

  # Output lines unless they are later overridden in PACKRATDATA.
  if (FILENAME != PACKRATDATA && line ~ /^[LZ]/) {
    overridden = 0
    split(line, field)
    overridden = packrat_zone[field[2 + (field[1] == "Link")]]
  }
  if (!overridden)
    print line
}
