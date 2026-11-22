#pragma once

#include <cstdint>
#include <vector>

#include "cli.h"

struct RunResult {
    double wall_ms = 0.0;
    double user_ms = 0.0;
    double sys_ms = 0.0;
    std:: uint64_t peak_rss_bytes = 0;
    int exit_code = 0;
    int term_signal = 0;
    bool timed_out = false;
};

std:: vector<RunResult> run_benchmark(const Config& cfg);
