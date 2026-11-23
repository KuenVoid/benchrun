#!/bin/sh
set -eu

benchrun=${1:?usage: smoke.sh BENCHRUN}
tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/benchrun-smoke.XXXXXX")
trap 'rm -rf "$tmp_dir"' EXIT

"$benchrun" --help >"$tmp_dir/help"
grep -q "Usage:" "$tmp_dir/help"

if "$benchrun" >"$tmp_dir/missing" 2>&1; then
    echo "missing command unexpectedly succeeded" >&2
    exit 1
fi
grep -q "missing command" "$tmp_dir/missing"

"$benchrun" -- /bin/sh -c 'exit 0' >"$tmp_dir/success"
if "$benchrun" -- /bin/sh -c 'exit 3' >"$tmp_dir/failure"; then
    echo "non-zero command unexpectedly succeeded" >&2
    exit 1
fi
grep -q "1 failed" "$tmp_dir/failure"

if "$benchrun" -t 50 -- /bin/sh -c 'sleep 2 & wait' >"$tmp_dir/timeout"; then
    echo "timeout unexpectedly succeeded" >&2
    exit 1
fi
grep -q "1 timed out" "$tmp_dir/timeout"
if pgrep -f '[s]leep 2' >/dev/null 2>&1; then
    echo "timed-out descendant is still running" >&2
    exit 1
fi
