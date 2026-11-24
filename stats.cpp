#include "stats.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <numeric>
#include <vector>

namespace {
MetricStats summarize_metric(std::vector<double>& values) {
    if (values.empty())
        return {};
    std::sort(values.begin(), values.end());
    MetricStats result;
    size_t n = values.size();
    result.min = values.front();
    result.max = values.back();
    result.median = (n % 2 == 0) ? (values[n / 2] + values[n / 2 - 1]) / 2 : values[n / 2];
    result.mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(n);
    if (n < 2) {
        result.stddev = 0.0;
    } else {
        const double sq_sum = std::transform_reduce(values.begin(), values.end(), 0.0, std::plus<>{}, [mean = result.mean](double x) {
            const double d = x - mean;
            return d * d;
        });
        result.stddev = std::sqrt(sq_sum / static_cast<double>(n - 1));
    }
    return result;
}

} // namespace

BenchmarkSummary compute_stats(std::span<const RunResult> results, AggregationPolicy policy) {
    BenchmarkSummary summary{};
    if (results.empty())
        return summary;

    std::vector<double> wall;
    std::vector<double> user;
    std::vector<double> sys;
    std::vector<double> rss;

    wall.reserve(results.size());
    user.reserve(results.size());
    sys.reserve(results.size());
    rss.reserve(results.size());

    summary.total_runs = results.size();
    for (const auto& r : results) {
        if (r.outcome == RunOutcome::timed_out) {
            ++summary.timed_out_runs;
        } else if (r.outcome != RunOutcome::succeeded) {
            ++summary.failed_runs;
        } else {
            ++summary.succeeded;
            if (policy == AggregationPolicy::successful_runs) {
                wall.push_back(r.wall_ms);
                user.push_back(r.user_ms);
                sys.push_back(r.sys_ms);
                rss.push_back(static_cast<double>(r.peak_rss_bytes));
            }
        }
    }

    summary.wall_ms = summarize_metric(wall);
    summary.user_ms = summarize_metric(user);
    summary.sys_ms = summarize_metric(sys);
    summary.peak_rss_bytes = summarize_metric(rss);

    return summary;
}
