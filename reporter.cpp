#include "reporter.h"

#include <array>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {
std::string format_duration(double ms) {
    if (ms < 0.0)
        ms = 0.0;
    double value = ms;
    std::string_view unit = " ms";

    if (ms < 1.0) {
        value = ms * 1000;
        unit = " us";
    } else if (ms >= 60000) {
        value = ms / 60000.0;
        unit = " min";
    } else if (ms >= 1000) {
        value = ms / 1000.0;
        unit = " s";
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value << unit;
    return oss.str();
}

std::string format_bytes(double bytes) {
    if (bytes < 0.0)
        bytes = 0.0;
    constexpr std::array<std::string_view, 4> units{" B", " KB", " MB", " GB"};
    std::size_t index = 0;
    while (bytes >= 1024 && index + 1 < units.size()) {
        bytes /= 1024;
        ++index;
    }

    std::ostringstream oss;
    if (index == 0)
        oss << static_cast<std::uint64_t>(bytes) << units[index];
    else
        oss << std::fixed << std::setprecision(2) << bytes << units[index];
    return oss.str();
}

void print_metric_row(std::string_view label, const MetricStats& m, bool is_bytes) {
    auto fmt = [is_bytes](double v) { return is_bytes ? format_bytes(v) : format_duration(v); };

    const std::string mean_std = fmt(m.mean) + " +- " + fmt(m.stddev);

    std::cout << std::left << std::setw(10) << label << std::setw(24) << mean_std << std::setw(14) << fmt(m.median) << std::setw(14)
              << fmt(m.min) << std::setw(14) << fmt(m.max) << '\n';
}

} // namespace

void print_report(const Config& cfg, const BenchmarkSummary& summary) {
    std::cout << "Benchmark: ";
    for (std::size_t i = 0; i < cfg.command.size(); ++i) {
        if (i > 0)
            std::cout << ' ';
        std::cout << cfg.command[i];
    }
    std::cout << " (" << cfg.runs << " runs, " << cfg.warmup << " warmup)\n";

    std::cout << "Status:    " << summary.succeeded << '/' << summary.total_runs << " succeeded";
    if (summary.failed_runs > 0 || summary.timed_out_runs > 0) {
        std::cout << " (" << summary.failed_runs << " failed, " << summary.timed_out_runs << " timed out)";
    }
    std::cout << "\n\n";

    std::cout << std::left << std::setw(10) << "Metric" << std::setw(24) << "Mean +- Stddev" << std::setw(14) << "Median"
              << std::setw(14) << "Min" << std::setw(14) << "Max" << '\n'
              << std::string(76, '-') << '\n';

    print_metric_row("Wall", summary.wall_ms, false);
    print_metric_row("User", summary.user_ms, false);
    print_metric_row("Sys", summary.sys_ms, false);
    print_metric_row("Peak RSS", summary.peak_rss_bytes, true);
}
