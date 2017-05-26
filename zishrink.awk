# Convert tzdata source into a smaller version of itself.

# Contributed by Paul Eggert.  This file is in the public domain.

# This is not a general-purpose converter; it is designed for current tzdata.
# 'zic' should treat this script's output as if it were identical to
# this script's input.

function paw_through_packratdata(line)
{
  if (PACKRATDATA) {
    while (0 < (getline line <PACKRATDATA)) {
      if (split(line, field)) {
	if (field[1] == "Zone") packrat_zone[field[2]] = 1
	if (field[1] == "Link") packrat_zone[field[3]] = 1
      }
    }
    close(PACKRATDATA)
  }
}

function process_input_line(line, field, end)
{
  # Remove comments, normalize spaces, and append a space to each line.
  sub(/#.*/, "", line)
  line = line " "
  gsub(/[[:space:]]+/, " ", line)

  # SystemV rules are not needed.
  if (line ~ /^Rule SystemV /) next

  # Replace FooAsia rules with the same rules without "Asia", as they
  # are duplicates.
  if (match(line, /[^ ]Asia /)) {
    if (line ~ /^Rule /) next
    line = substr(line, 1, RSTART) substr(line, RSTART + 5)
  }

  # Abbreviate times.
  while (match(line, /[: ]0+[0-9]/))
    line = substr(line, 1, RSTART) substr(line, RSTART + RLENGTH - 1)
  while (match(line, /:0[^:]/))
    line = substr(line, 1, RSTART - 1) substr(line, RSTART + 2)

  # Abbreviate weekday names.  Do not abbreviate "Sun" and "Sat", as
  # pre-2017c zic erroneously diagnoses "Su" and "Sa" as ambiguous.
  while (match(line, / (last)?(Mon|Wed|Fri)[ <>]/)) {
    end = RSTART + RLENGTH
    line = substr(line, 1, end - 4) substr(line, end - 1)
  }
  while (match(line, / (last)?(Tue|Thu)[ <>]/)) {
    end = RSTART + RLENGTH
    line = substr(line, 1, end - 3) substr(line, end - 1)
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

  # Remove unnecessary trailing days-of-month "1".
  if (match(line, /[[:alpha:]] 1$/))
    line = substr(line, 1, RSTART)

  # Remove unnecessary trailing " Ja" (for January).
  sub(/ Ja$/, "", line)

  # Output lines unless they are later overridden in PACKRATDATA.
  if (line ~ /^[LRZ]/) {
    overridden = 0
    if (FILENAME != PACKRATDATA) {
      split(line, field)
      if (field[1] == "Zone")
	overridden = packrat_zone[field[2]]
      else if (field[1] == "Link" && packrat_zone[field[3]])
	next
    }
  }
  if (!overridden)
    print line
}

BEGIN {
  print "# This zic input file is in the public domain."
  paw_through_packratdata()
}

/^[[:space:]]*[^#[:space:]]/ {
  process_input_line($0)
}
