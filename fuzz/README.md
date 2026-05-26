# tz Fuzz Harnesses

Coverage-guided fuzz harnesses for the tz (tzdata) TZif binary format parser.

## Harnesses

### `fuzz_tzif.c`

Targets `tzloadbody()` in `localtime.c` — the parser for TZif version 1/2/3
binary timezone data files.

**Why this matters:**

Any process that calls `tzset(3)` with a TZ environment variable pointing to
a file (e.g. `TZ=:/path/to/file`) will call `tzloadbody()` on that file's
contents. This is exploitable when:
- An attacker can write a file that ends up in TZ (e.g. via container mounts)
- An application calls `tzalloc()` with user-controlled input
- A crafted `/usr/share/zoneinfo/` entry is installed

Bugs in `detzcode()`/`detzcode64()` result handling, array bounds checks on
`ttis[]`, `leaps[]`, `chars[]`, or version-2/3 header parsing could cause
out-of-bounds reads in privileged processes.

## Building

```bash
clang -fsanitize=fuzzer,address \
      fuzz/fuzz_tzif.c localtime.c asctime.c difftime.c strftime.c \
      -o fuzz_tzif

./fuzz_tzif -max_total_time=60
```

## Seed Corpus

Use real TZif files from `/usr/share/zoneinfo/` as a starting corpus:

```bash
mkdir corpus/
cp /usr/share/zoneinfo/America/New_York corpus/
cp /usr/share/zoneinfo/Europe/London corpus/
cp /usr/share/zoneinfo/Asia/Tokyo corpus/
cp /usr/share/zoneinfo/UTC corpus/
./fuzz_tzif corpus/ -max_total_time=300
```
