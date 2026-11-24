#!/bin/sh
set -eu

benchrun=${1:?usage: smoke.sh BENCHRUN}
# Resolve the executable before any test changes directory.
benchrun_dir=$(CDPATH= cd -- "$(dirname "$benchrun")" && pwd)
benchrun="$benchrun_dir/$(basename "$benchrun")"
tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/benchrun-smoke.XXXXXX")
trap 'rm -rf "$tmp_dir"' EXIT

"$benchrun" --help >"$tmp_dir/help"
grep -q "Usage:" "$tmp_dir/help"

"$benchrun" -h >"$tmp_dir/help-short"
grep -q "Usage:" "$tmp_dir/help-short"

if "$benchrun" >"$tmp_dir/missing" 2>&1; then
    echo "missing command unexpectedly succeeded" >&2
    exit 1
fi
grep -q "missing command" "$tmp_dir/missing"

"$benchrun" -n 2 -- /bin/echo success >"$tmp_dir/arg-mode"
grep -q "2/2 succeeded" "$tmp_dir/arg-mode"

"$benchrun" -r 2 -- /bin/echo success >"$tmp_dir/runs-short"
grep -q "2/2 succeeded" "$tmp_dir/runs-short"

"$benchrun" --runs 2 -- /bin/echo success >"$tmp_dir/runs-long"
grep -q "2/2 succeeded" "$tmp_dir/runs-long"

"$benchrun" --warmup 1 --runs 2 -- /bin/echo success >"$tmp_dir/warmup-runs-long"
grep -q "2/2 succeeded" "$tmp_dir/warmup-runs-long"
grep -q "(2 runs, 1 warmup)" "$tmp_dir/warmup-runs-long"

"$benchrun" --shell -- printf 'shell-mode-ok' >"$tmp_dir/shell"
grep -q "shell-mode-ok" "$tmp_dir/shell"

cat >"$tmp_dir/quiet-command" <<'EOF'
#!/bin/sh
printf 'should-be-hidden\n'
EOF
chmod +x "$tmp_dir/quiet-command"
"$benchrun" -q -- "$tmp_dir/quiet-command" >"$tmp_dir/quiet"
if grep -q "should-be-hidden" "$tmp_dir/quiet"; then
    echo "quiet mode leaked child stdout" >&2
    exit 1
fi

"$benchrun" -w 1 -n 1 -- /bin/sh -c 'exit 0' >"$tmp_dir/warmup"
grep -q "1/1 succeeded" "$tmp_dir/warmup"

"$benchrun" --verbose -- /bin/sh -c 'exit 0' >"$tmp_dir/verbose"
grep -q "Per-run results:" "$tmp_dir/verbose"
grep -q "run 1: succeeded" "$tmp_dir/verbose"

(cd "$tmp_dir" && "$benchrun" --json -- /bin/sh -c 'exit 0') >"$tmp_dir/json-terminal"
grep -q "Report written to report.json" "$tmp_dir/json-terminal"
grep -q '"schema_version":1' "$tmp_dir/report.json"
grep -q '"outcome":"succeeded"' "$tmp_dir/report.json"
grep -Eq '"wall_ms":\{"min":[1-9][0-9]*([.][0-9]+)?' "$tmp_dir/report.json"
grep -Eq '"wall_ms":.*"max":[1-9][0-9]*([.][0-9]+)?' "$tmp_dir/report.json"

(cd "$tmp_dir" && "$benchrun" --runs-json -- /bin/sh -c 'exit 0') >"$tmp_dir/runs-json-terminal"
grep -q "Report written to report.json" "$tmp_dir/runs-json-terminal"
grep -q '"schema_version":1' "$tmp_dir/report.json"
grep -q '"runs":' "$tmp_dir/report.json"

(cd "$tmp_dir" && "$benchrun" --csv -- /bin/sh -c 'exit 0') >"$tmp_dir/csv-terminal"
grep -q "Report written to report.csv" "$tmp_dir/csv-terminal"
grep -q 'run_index,phase,outcome' "$tmp_dir/report.csv"
grep -q 'measured,succeeded' "$tmp_dir/report.csv"

"$benchrun" --cwd "$tmp_dir" --env BENCHRUN_SMOKE=ok --clear-env --null-stdin -- \
    /bin/sh -c 'test "$BENCHRUN_SMOKE" = ok && test -d . && test ! -t 0' \
    >"$tmp_dir/controls"

printf 'fixture-input' >"$tmp_dir/stdin"
"$benchrun" --stdin-file "$tmp_dir/stdin" -- /bin/sh -c 'test "$(cat)" = fixture-input' \
    >"$tmp_dir/stdin-test"
grep -q "1/1 succeeded" "$tmp_dir/stdin-test"

if "$benchrun" -m 1 -- /bin/sh -c 'exit 0' >"$tmp_dir/memory-limit" 2>&1; then
    echo "memory limit unexpectedly allowed /bin/sh" >&2
    exit 1
fi
grep -Eq 'setrlimit|failed|cannot allocate|Error:' "$tmp_dir/memory-limit"

"$benchrun" -- /bin/sh -c 'exit 0' >"$tmp_dir/success"
if "$benchrun" -- /bin/sh -c 'exit 3' >"$tmp_dir/failure"; then
    echo "non-zero command unexpectedly succeeded" >&2
    exit 1
fi
grep -q "1 failed" "$tmp_dir/failure"

if "$benchrun" -- /bin/sh -c 'kill -TERM $$' >"$tmp_dir/signal"; then
    echo "signaled command unexpectedly succeeded" >&2
    exit 1
fi
grep -q "1 failed" "$tmp_dir/signal"

if "$benchrun" -t 50 -- /bin/sh -c 'sleep 2 & wait' >"$tmp_dir/timeout"; then
    echo "timeout unexpectedly succeeded" >&2
    exit 1
fi
grep -q "1 timed out" "$tmp_dir/timeout"
if pgrep -f '[s]leep 2' >/dev/null 2>&1; then
    echo "timed-out descendant is still running" >&2
    exit 1
fi

# Optional custom-program benchmark checks. This block is intentionally
# commented so the generic smoke suite has no repository-specific dependency.
# Contributors can set the executable and arguments for their own program.
#
# custom_program=/path/to/custom-program
# custom_args="argument1 argument2"
#
# "$benchrun" -r 2 -w 1 -- "$custom_program" $custom_args >"$tmp_dir/custom-r-w"
# grep -q "2/2 succeeded" "$tmp_dir/custom-r-w"
# grep -q "(2 runs, 1 warmup)" "$tmp_dir/custom-r-w"
#
# "$benchrun" -w 1 -n 2 --verbose -- "$custom_program" $custom_args >"$tmp_dir/custom-verbose"
# grep -q "warmup 1: succeeded" "$tmp_dir/custom-verbose"
# grep -q "run 1: succeeded" "$tmp_dir/custom-verbose"
# grep -q "2/2 succeeded" "$tmp_dir/custom-verbose"
#
# "$benchrun" -n 2 --json -- "$custom_program" $custom_args >"$tmp_dir/custom-json"
# grep -q '"status":{"total":2,"succeeded":2,"failed":0,"timed_out":0}' "$tmp_dir/custom-json"
# grep -q '"aggregation":"successful_runs"' "$tmp_dir/custom-json"
#
# "$benchrun" -n 2 --runs-json -- "$custom_program" $custom_args >"$tmp_dir/custom-runs-json"
# grep -q '"phase":"measured"' "$tmp_dir/custom-runs-json"
# grep -q '"outcome":"succeeded"' "$tmp_dir/custom-runs-json"
#
# "$benchrun" -n 2 --csv -- "$custom_program" $custom_args >"$tmp_dir/custom-csv"
# grep -q '0,measured,succeeded' "$tmp_dir/custom-csv"
# grep -q '1,measured,succeeded' "$tmp_dir/custom-csv"
#
# "$benchrun" --shell -- "$custom_program" $custom_args >"$tmp_dir/custom-shell"
# grep -q "1/1 succeeded" "$tmp_dir/custom-shell"
