#pragma once

#include <vector>

#include "cli.h"
#include "runner.h"
#include "stats.h"

void render_report(const Config& cfg, const std:: vector<RunResult>& results, const BenchmarkSummary& summary);
