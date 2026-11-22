#pragma once

#include <cstddef>
#include <vector>

#include "runner.h"

struct MetricStats {
    double min = 0.0;
    double max = 0.0;
    double mean = 0.0;
    double median = 0.0;
};

struct BenchmarkSummary {
    MetricStats wall_ms;
    MetricStats user_ms;
    MetricStats sys_ms;
    MetricStats peak_rss_mb;
    std:: size_t total_runs = 0;
    std:: size_t failed_runs = 0;
    std:: size_t timed_out_runs = 0;
};

BenchmarkSummary compute_stats(const std:: vector<RunResult>& results);
