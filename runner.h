#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "cli.h"

/** Final command outcome for one benchmark run. */
enum class RunOutcome {
    succeeded,
    exited_nonzero,
    signaled,
    timed_out,
};

/** Complete measured and process-status data for one warmup or measured run. */
struct RunResult {
    std::size_t run_index = 0;
    bool warmup = false;
    std::uint64_t start_time_ns = 0;
    std::uint64_t end_time_ns = 0;
    double wall_ms = 0.0;
    double user_ms = 0.0;
    double sys_ms = 0.0;
    std::uint64_t peak_rss_bytes = 0;
    int exit_code = -1;
    int term_signal = 0;
    RunOutcome outcome = RunOutcome::succeeded;
};

/** Infrastructure failure encountered while preparing or executing a run. */
struct RunnerError {
    std::string operation;
    int error_code = 0;
    std::string message;
};

using RunExecution = std::variant<RunResult, RunnerError>;

/** Ordered warmup and measured run records produced by a benchmark. */
struct BenchmarkResults {
    std::vector<RunResult> warmups;
    std::vector<RunResult> measured;
};

/** Benchmark records plus an optional infrastructure-level runner failure. */
struct BenchmarkExecution {
    BenchmarkResults results;
    std::optional<RunnerError> error;
};

/** Injectable execution boundary for benchmark run implementations. */
class CommandExecutor {
public:
    /** Releases platform resources owned by the executor implementation. */
    virtual ~CommandExecutor() = default;

    /**
     * Executes one run.
     * run_index identifies the run within its warmup or measured phase;
     * warmup distinguishes warmup from measured execution.
     */
    virtual RunExecution execute(std::size_t run_index, bool warmup) = 0;
};

/**
 * Runs a benchmark using the POSIX CommandExecutor implementation.
 * Executes warmups first, then measured runs, and stops on a runner error.
 */
BenchmarkExecution run_benchmark(const Config& cfg);

/**
 * Runs a benchmark using a caller-provided executor.
 * This overload allows warmup, measured-run, and runner-error behavior to be
 * tested without launching real processes.
 */
BenchmarkExecution run_benchmark(const Config& cfg, CommandExecutor& executor);
