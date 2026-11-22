# benchrun

A lightweight, zero-dependency C++20 CLI tool for multi-run execution timing, peak RAM (RSS) tracking, and resource-capped benchmarking on Linux and macOS.

Built to fill the gap between `/usr/bin/time` (single-run, inconsistent OS flags) and `hyperfine` (multi-run timing without memory tracking or resource caps).

---

## Features (v0.0)

* **Cross-Platform POSIX Metrics:** Consistent flags and normalized output across Linux and macOS (handles `ru_maxrss` KB vs. bytes differences automatically).
* **Multi-Run Statistics:** Supports warmup runs (`-w`) and measured iterations (`-n`), reporting min, max, mean, and median for wall time, user CPU, sys CPU, and peak RAM.
* **Resource Guardrails:** Enforce hard execution timeouts (`-t`) and memory limits (`-m` via `setrlimit`).
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
| `-m, --max-mem <MB>` | `0` (off) | Cap child address space via `setrlimit(RLIMIT_AS)` |
| `--json` | `false` | Output benchmark results in JSON format |
| `-h, --help` | -- | Display usage information |

---

## Build & Install

Requires a C++20 compiler (`g++` or `clang++`) on Linux or macOS.

```bash
# Compile
g++ -std=c++20 -Wall -Wextra -Wpedantic -O2 \
    src/main.cpp src/cli.cpp src/runner.cpp src/stats.cpp src/reporter.cpp \
    -o benchrun

# Install globally for your user (no sudo required)
mkdir -p ~/.local/bin
cp benchrun ~/.local/bin/benchrun


Ensure `export PATH="$HOME/.local/bin:$PATH"` is in your `.bashrc` or `.zshrc`.

---

## Architecture

```text
src/
├── config.h / config.cpp      # Declarative flag parser & help generator
├── runner.h / runner.cpp      # POSIX fork/execvp/wait4, rusage, timeout & setrlimit
├── stats.h / stats.cpp        # Min, max, mean, median & failure aggregation
├── reporter.h / reporter.cpp  # Terminal table & JSON renderer
└── main.cpp                   # Top-level orchestration pipeline
