#include "runner.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <csignal>
#include <chrono>
#include <cstdint>

namespace {

struct PreparedCommand {
    std::string shell_cmd;
    std::vector<char*> argv;
};

PreparedCommand prepare_command(const Config& cfg);

std::uint64_t normalize_max_rss_kb(long ru_maxrss);

void setup_child_process(const Config& cfg);

[[noreturn]] void exec_command(const PreparedCommand& cmd);

int wait_for_child(pid_t pid, std::uint64_t timeout_ms, rusage& ru, bool& timed_out);

RunResult run_once(const Config& cfg, const PreparedCommand& cmd);

} // Namespace End

std:: vector<RunResult> run_benchmark(const Config& cfg);
