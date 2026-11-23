# benchrun
$\color{grey}\text{Code Assisted By AI}$

A lightweight, zero-dependency C++20 CLI tool for multi-run execution timing, peak RSS tracking, and resource-capped benchmarking on Linux and macOS.

Built to fill the gap between `/usr/bin/time` (single-run, inconsistent OS flags) and `hyperfine` (multi-run timing without memory tracking or resource caps).

---

## Features (v0.0)

* **Cross-Platform POSIX Metrics:** Consistent flags and normalized output across Linux and macOS (handles `ru_maxrss` KB vs. bytes differences automatically).
* **Multi-Run Statistics:** Supports warmup runs (`-w`) and measured iterations (`-n`), reporting min, max, mean, and median for successful runs.
* **Resource Guardrails:** Enforce hard execution timeouts (`-t`) and virtual address-space limits (`-m` via `setrlimit`).
* **Clean Reporting:** Human-readable terminal summary by default, plus `--json` for scripting and logging.

---

## Usage

```bash
# Single run
benchrun ./my_program arg1 arg2

# 10 measured runs with 2 warmups, muting program stdout/stderr
benchrun -n 10 -w 2 -q ./my_sort 100000

# Enforce a 2-second timeout and 256 MB RAM cap
benchrun -t 2000 -m 256 ./my_program

# Run raw shell syntax (arguments are joined and passed to /bin/sh -c)
benchrun --shell -- printf 'hello\n' '|' wc -c

# Use explicit '--' when the target program has colliding flags
benchrun -n 5 -- ./my_app -n 100 -q
```

### CLI Options

| Flag | Default | Description |
|---|---|---|
| `-n, --runs <int>` | `1` | Number of measured executions |
| `-w, --warmup <int>` | `0` | Unmeasured warmup runs before timing |
| `-q, --quiet` | `false` | Redirect child `stdout`/`stderr` to `/dev/null` |
| `-s, --shell` | `false` | Execute command via `/bin/sh -c` (supports pipes/redirects) |
| `-t, --timeout <ms>` | `0` (off) | Kill child with `SIGKILL` if execution exceeds limit |
| `-m, --max-mem <MB>` | `0` (off) | Cap child virtual address space via `setrlimit(RLIMIT_AS)` |
| `--json` | `false` | Output benchmark results in JSON format |
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

Requires a C++20 compiler (`g++` or `clang++`) on Linux or macOS.

```bash
# Configure and build
cmake -S . -B build
cmake --build build

# Install globally for your user (no sudo required)
mkdir -p ~/.local/bin
cp build/benchrun ~/.local/bin/benchrun
```

Ensure `export PATH="$HOME/.local/bin:$PATH"` is in your `.bashrc` or `.zshrc`.

Run the smoke tests with `ctest --test-dir build --output-on-failure`.

See [TODO.md](TODO.md) for the development roadmap and [Report.md](Report.md)
for the implementation summary and remaining error-handling concerns.

---

## Architecture

```text
├── cli.h / cli.cpp            # Declarative flag parser & help generator
├── runner.h / runner.cpp      # POSIX fork/execvp/wait4, rusage, timeout & setrlimit
├── stats.h / stats.cpp        # Min, max, mean, median & failure aggregation
├── reporter.h / reporter.cpp  # Terminal table & JSON renderer
└── main.cpp                   # Top-level orchestration pipeline
```
