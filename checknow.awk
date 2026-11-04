# Check zonenow.tab for consistency with primary data.

# Contributed by Paul Eggert.  This file is in the public domain.

function record_zone(zone, data) {
  if (zone) {
    zone_data[zone] = data
    zones[data] = zones[data] " " zone
  }
}

BEGIN {
  while (getline <zdump_table) {
    if ($0 ~ /^TZ/) {
      record_zone(zone, data)
      zone = $0
      sub(/.*\.ckd\//, "", zone)
      sub(/\/\//, "/", zone)
      sub(/"/, "", zone)
      data = ""
    } else if ($0 ~ /./)
      data = data $0 "\n"
  }
  record_zone(zone, data)
  FS = "\t"
}

/^[^#]/ {
  zone = $3
  data = zone_data[zone]
  if (!data) {
    printf "%s: no data\n", zone
    status = 1
  } else {
    zone2 = zonenow[data]
    if (zone2) {
      printf "zones %s and %s identical from now on\n", zone, zone2
      identical = status = 1
    } else
      zonenow[data] = zone
  }
}

END {
 for (zone in zone_data) {
    data = zone_data[zone]
    if (data && !zonenow[data]) {
      printf "Zone table should have one of:%s\n", zones[data]
      zonenow[data] = zone # This suppresses duplicate diagnostics.
      status = 1
    }
 }
 if (now_out) {
   for (data in zones) {
     printf "%s=%s\n", now, zones[data] >>now_out
   }
 }
 if (identical) {
   print ""
   print "To suppress \"identical from now on\" failures"
   print "you can set CHECK_NOW_TIMESTAMP to spoof the clock, e.g.:"
   print ""
   print "  release_time=$(awk '/^Release /{print $4 \"T\" $5 $6; exit}' NEWS)"
   print "  make CHECK_NOW_TIMESTAMP=$(date -d$release_time +%s) check"
   print ""
   print "Better yet, you can remove the duplicates from zonenow.tab."
 }
 exit status
}
