# Architectural Rules

- Keep process lifecycle logic in [runner.cpp](runner.cpp) and expose only the
  execution boundary and result types through [runner.h](runner.h).
- Keep CLI parsing and configuration validation behind [cli.h](cli.h);
  do not make other modules depend on parser implementation details.
- Keep statistical aggregation behind [stats.h](stats.h); it should consume
  run results and remain independent of process-launching and rendering code.
- Keep terminal, JSON, and CSV rendering behind [reporter.h](reporter.h).
  Split focused rendering helpers or modules if [reporter.cpp](reporter.cpp)
  grows substantially.
- Keep [main.cpp](main.cpp) as the top-level orchestration layer: parse and
  validate configuration, execute the benchmark, aggregate results, render the
  report, and return the appropriate status.
- Preserve the existing result model: command outcomes belong in
  `RunResult`, while preparation or execution infrastructure failures belong
  in `RunnerError`.
- Keep platform-specific process and resource-limit behavior in the runner;
  expose normalized, stable result values to statistics and reporting.
- Use C++ unit tests to complement the shell smoke suite. Do not add
  repository-specific executables to the default test path.
