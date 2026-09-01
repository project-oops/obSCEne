#!/bin/sh
# Summarises a census run: how much of each library's known surface is present.
# Exists because the interesting number is a ratio, and counting it by hand from a
# few hundred sym records is exactly the sort of thing that gets done wrong once.
set -e
report="${1:-/tmp/obs/host-report.txt}"
total=$(grep -c '^OBS|sym|' "$report" || true)
present=$(grep '^OBS|sym|' "$report" | grep -c '|present|' || true)
echo "census symbols : $total"
echo "present        : $present"
echo "absent         : $((total - present))"
echo
# An absence in the other generation is expected and is not a gap. Counting the two
# together is what makes a coverage figure meaningless.
shared_total=$(grep '^OBS|sym|' "$report" | grep -c '|shared$' || true)
shared_present=$(grep '^OBS|sym|' "$report" | grep '|present|shared$' -c || true)
other=$(grep '^OBS|sym|' "$report" | grep -cE '\|(previous|current)$' || true)
unknown=$(grep '^OBS|sym|' "$report" | grep -c '|unknown$' || true)
echo "--- by generation ---"
echo "shared         : $shared_present / $shared_total present   <- the real coverage number"
echo "other console  : $other  (absence here is expected, not a gap)"
echo "unclassified   : $unknown"
echo
echo "--- per library ---"
grep '^OBS|res|900-surface' "$report" | while IFS='|' read -r _ _ id status value detail; do
  printf '  %-34s %-8s %s %s\n' "$id" "$status" "$value" "$detail"
done
echo
echo "--- final tally ---"
grep '^OBS|tally|' "$report"
