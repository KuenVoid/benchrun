# benchrun

[![CI](https://github.com/KuenVoid/benchrun/actions/workflows/ci.yml/badge.svg)](https://github.com/KuenVoid/benchrun/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://isocpp.org/)

**Lightweight, zero-dependency C++20 CLI for multi-run timing, Peak RSS tracking, and resource-capped benchmarking on Linux and macOS.**

`benchrun` bridges the gap between `/usr/bin/time` (single-run, OS-inconsistent flags) and `hyperfine` (multi-run timing without memory metrics or resource caps).

## Demo

```bash
$ benchrun -n 10 -w 2 -t 2000 -m 256 -- ./my_algo < input.txt
```
Example output (`benchrun -w 5 -n 10 ./nlp`):

<img width="592" height="135" alt="Screenshot 2026-11-25 at 2 21 42 PM" src="https://github.com/user-attachments/assets/6b0ada1d-1b6b-4493-b6f8-9edc3216256c" />

## Why benchrun?

| Feature | `/usr/bin/time` | `hyperfine` | `benchrun` |
|---|---|---|---|
| Multi-run stats + warmups | No | Yes | Yes |
| Peak RSS tracking (Linux KB / macOS bytes normalized) | Yes (OS-dependent) | No | Yes |
| Timeout (`-t`) & memory caps (`-m`) | No | No | Yes |
| Execution env controls (`--cwd`, `--env`, `--stdin-file`) | No | Partial | Yes |
| JSON / CSV export | No | Yes | Yes |
| Zero external dependencies | Yes | No (Rust crates) | Yes (C++20 + POSIX) |

---

## Usage

```bash
# Single run
benchrun ./my_program arg1 arg2

# 10 measured runs with 2 warmups, muting program stdout/stderr
benchrun -n 10 -w 2 -q ./my_sort 100000

# Enforce a 2-second timeout and a 256 MB virtual-address-space limit
benchrun -t 2000 -m 256 ./my_program

# Run raw shell syntax (arguments are joined and passed to /bin/sh -c)
benchrun --shell -- printf 'hello\n' '|' wc -c

# Use explicit '--' when the target program has colliding flags
benchrun -n 5 -- ./my_app -n 100 -q

# Machine-readable output
benchrun --json -n 5 -- ./my_app
benchrun --runs-json -- ./my_app
benchrun --csv -n 5 -- ./my_app
```

Machine-readable modes write files in the directory where `benchrun` is
invoked:

- `--json` writes the aggregate report and per-run records to `report.json`.
- `--runs-json` writes only the per-run JSON records to `report.json`.
- `--csv` writes the measured-run records to `report.csv`.

Running `--json` or `--runs-json` replaces an existing `report.json`. Running
`--csv` replaces an existing `report.csv`. These modes print a file-written
confirmation rather than the machine-readable report itself.

### Execution environment
```bash
benchrun --cwd /tmp --env MODE=test --clear-env --null-stdin -- ./my_app
```

### CLI Options

| Flag | Default | Description |
|---|---|---|
| `-n, -r, --runs <int>` | `1` | Number of measured executions |
| `-w, --warmup <int>` | `0` | Unmeasured warmup runs before timing |
| `-q, --quiet` | `false` | Redirect child `stdout`/`stderr` to `/dev/null` |
| `-s, --shell` | `false` | Execute command via `/bin/sh -c` (supports pipes/redirects) |
| `-t, --timeout <ms>` | `0` (off) | Kill child with `SIGKILL` if execution exceeds limit |
| `-m, --max-mem <MB>` | `0` (off) | Cap child virtual address space via `setrlimit(RLIMIT_AS)` |
| `--json` | `false` | Write benchmark results to `report.json` |
| `--verbose` | `false` | Print individual warmup and measured run results |
| `--runs-json` | `false` | Write only schema-versioned per-run JSON records to `report.json` |
| `--csv` | `false` | Write measured per-run records to `report.csv` |
| `--cwd <path>` | inherited | Set the child working directory |
| `--env <KEY=VALUE>` | inherited | Set an environment variable (repeatable) |
| `--clear-env` | `false` | Clear the inherited environment before applying `--env` |
| `--stdin-file <path>` | inherited | Read child stdin from a file |
| `--null-stdin` | `false` | Connect child stdin to `/dev/null` |
| `-h, --help` | -- | Display usage information |

Shell mode treats the command arguments as raw `/bin/sh` syntax; it does not
quote or escape individual arguments. Use normal shell quoting when invoking
`benchrun`, and use normal argv mode when arguments must be passed literally.

Each measured wall-clock value starts after `fork()` returns in the parent and
ends when the child is reaped, so it includes child setup, process startup,
command execution, wait scheduling, and reaping. The timeout uses a
deadline-aware portable polling fallback. The child inherits the current
working directory, environment, stdin, and non-quiet standard output/error;
each run is placed in its own process group so timeout cleanup includes
descendants.

---

## Build & Install

Requires a C++20 compiler on Linux or macOS.

### Method 1: CMake

```bash
cmake -S . -B build
cmake --build build --parallel

# Run focused unit tests and the end-to-end smoke suite
ctest --test-dir build --output-on-failure
```

With sanitizers enabled:

```bash
cmake -S . -B build-sanitize -DBENCHRUN_ENABLE_SANITIZERS=ON
cmake --build build-sanitize --parallel
ctest --test-dir build-sanitize --output-on-failure
```

Install globally from the CMake build (no sudo required):

```bash
mkdir -p ~/.local/bin
cp build/benchrun ~/.local/bin/benchrun
```

### Method 2: Manual compiler command

Use `g++`:

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic \
  main.cpp cli.cpp runner.cpp stats.cpp reporter.cpp \
  -o benchrun
./benchrun --help
tests/smoke.sh ./benchrun
```

Or use `clang++`:

```bash
clang++ -std=c++20 -Wall -Wextra -Wpedantic \
  main.cpp cli.cpp runner.cpp stats.cpp reporter.cpp \
  -o benchrun
```

Ensure `export PATH="$HOME/.local/bin:$PATH"` is in your `.bashrc` or `.zshrc`

```
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
```

after a CMake installation.
To check whether it is installed globally, please run:
```
which benchrun # should print ~/.local/bin/benchrun
```

### Smoke tests and custom programs

The default smoke suite is dependency-light and uses only the system shell,
standard POSIX utilities, and the compiled `benchrun` executable. It exercises
the CLI flags, output formats, execution controls, failure outcomes, timeout
cleanup, and JSON/CSV field structure:

```bash
# Direct compiler build
g++ -std=c++20 -Wall -Wextra -Wpedantic \
  main.cpp cli.cpp runner.cpp stats.cpp reporter.cpp \
  -o benchrun
tests/smoke.sh ./benchrun

# CMake build
ctest --test-dir build --output-on-failure
```

The final section of `tests/smoke.sh` contains an optional, commented
custom-program template. Set `custom_program` to your executable and
`custom_args` to its arguments, then uncomment the checks you need. It is
commented by default so contributors can run the generic suite without
depending on a repository-specific test program.

See [TODO.md](TODO.md) for the development roadmap

---

## Architecture

```
├── cli.h / cli.cpp            # Declarative flag parser & help generator
├── runner.h / runner.cpp      # POSIX fork/execvp/wait4, rusage, timeout & setrlimit
├── stats.h / stats.cpp        # Min, max, mean, median & failure aggregation
├── reporter.h / reporter.cpp  # Terminal table & JSON renderer
└── main.cpp                   # Top-level orchestration pipeline
```

---

## Execution Environment Controls

Use these abstract controls to run the benchmark in a predictable environment
without modifying the target program.

### `--cwd <path>`

Changes the working directory of the child process before it starts:

```bash
benchrun --cwd /tmp -- ./my_app
```

Relative paths used by the benchmarked program are resolved from `/tmp`.

### `--env <KEY=VALUE>`

Adds or replaces an environment variable in the child process. Repeat the flag
to set multiple variables:

```bash
benchrun --env MODE=test --env REGION=local -- ./my_app
```

Use `--clear-env` with `--env` when the benchmark should start without most
inherited environment variables:

```bash
benchrun --clear-env --env MODE=test -- ./my_app
```

### `--stdin-file <path>`

Connects the child process's standard input to a file:

```bash
benchrun --stdin-file fixture-input.txt -- ./my_app
```

The file is opened by the child process. It cannot be combined with
`--null-stdin`.

### `--null-stdin`

Connects the child process's standard input to `/dev/null`:

```bash
benchrun --null-stdin -- ./my_app
```

Use this when the benchmarked program should not read interactive input or
inherit terminal input. It cannot be combined with `--stdin-file`.

These options can be combined. For example:

```bash
benchrun --cwd /tmp --clear-env --env MODE=test \
  --null-stdin -- ./my_app
```

---

## Explanation of Statistics

benchrun reports each timing metric from the successful measured runs. Warmup
runs and failed, signaled, or timed-out executions are excluded from timing
statistics.

| Metric | Meaning | How it is measured |
|---|---|---|
| `Wall` | Real elapsed time, including process startup and waiting for the command to finish | Measured with `std::chrono::steady_clock` before waiting for the child and after `wait4` reaps it |
| `User` | CPU time spent by the child process in user mode | Read from `ru_utime` returned by POSIX `wait4` and converted to milliseconds |
| `Sys` | CPU time spent by the child process in kernel mode | Read from `ru_stime` returned by POSIX `wait4` and converted to milliseconds |
| `Peak RSS` | Maximum resident memory used by the child process | Read from `ru_maxrss` returned by POSIX `wait4`, then normalized to bytes |

Wall time and CPU time measure different kinds of time. `Wall` includes time
spent waiting for the scheduler, I/O, locks, or other system activity. `User`
and `Sys` count CPU execution time in user mode and kernel mode respectively.
Consequently, wall time is not required to equal `User + Sys`.

For every metric, the table reports:

* **Mean**: the arithmetic average across successful runs.
* **Stddev**: the sample standard deviation across successful runs.
* **Median**: the middle value when the run values are sorted.
* **Min**: the smallest value across successful runs.
* **Max**: the largest value across successful runs.

The report uses median for even-sized samples as the average of the two middle
values. One successful run has a standard deviation of `0`.

---

## Contributing

`benchrun` is an open-source learning and systems programming project, students and C++ enthusiasts are very welcome! Check out the [good first issues](https://github.com/KuenVoid/benchrun/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22) or [ARCHITECTURE.md](ARCHITECTURE.md) to get started.

