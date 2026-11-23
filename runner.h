#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "cli.h"

enum class RunOutcome {
    succeeded,
    exited_nonzero,
    signaled,
    timed_out,
};

struct RunResult {
    std::size_t run_index = 0;
    bool warmup = false;
    double wall_ms = 0.0;
    double user_ms = 0.0;
    double sys_ms = 0.0;
    std::uint64_t peak_rss_bytes = 0;
    int exit_code = -1;
    int term_signal = 0;
    RunOutcome outcome = RunOutcome::succeeded;
};

struct RunnerError {
    std::string operation;
    int error_code = 0;
    std::string message;
};

using RunExecution = std::variant<RunResult, RunnerError>;

struct BenchmarkResults {
    std::vector<RunResult> warmups;
    std::vector<RunResult> measured;
};

struct BenchmarkExecution {
    BenchmarkResults results;
    std::optional<RunnerError> error;
};

class CommandExecutor {
public:
    virtual ~CommandExecutor() = default;
    virtual RunExecution execute(std::size_t run_index, bool warmup) = 0;
};

BenchmarkExecution run_benchmark(const Config& cfg);
BenchmarkExecution run_benchmark(const Config& cfg, CommandExecutor& executor);
