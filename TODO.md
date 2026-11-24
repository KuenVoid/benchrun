# TODO

This list contains planned work that is not currently implemented.

## Benchmark controls

- Add configurable timeout escalation: send `SIGTERM`, wait for a configurable
  grace period, then send `SIGKILL`.
- Add user-selectable termination signals.
- Add adaptive repetition modes with minimum and maximum sample counts and
  elapsed-time targets.
- Add CPU-affinity and scheduling-priority controls.
- Add explicit warm-cache and cold-cache setup modes.

## Resource limits

- Evaluate resident-memory, open-file, and subprocess limits for Linux and
  macOS.
- Detect unsupported limit types before process launch.

## Reporting and statistics

- Include machine-readable runner-error records in JSON and CSV output.
- Report infrastructure failures alongside command outcomes in standard run
  coverage.
- Add percentiles, confidence intervals, trimmed means, and outlier reporting.
- Document stable units for every machine-readable field.
- Add baseline JSON comparison with configurable regression thresholds.
- Add optional structured exporters such as Prometheus or SQLite.

## Testing

- Add C++ unit tests for CLI parsing and configuration validation.
- Add fake-executor tests for warmups, measured runs, and runner failures.
- Add aggregation tests for empty, single-run, and even/odd median samples.
- Add platform integration tests for resource-limit behavior on Linux and
  macOS, including unsupported limit types and platform-specific units.
