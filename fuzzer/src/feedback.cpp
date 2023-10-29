#include "feedback.hpp"

#include <iostream>
#include <regex>

Feedback::Feedback(const bool is_success,
                   const std::vector<std::uint8_t> &stdout_buffer,
                   const std::vector<std::uint8_t> &stderr_buffer,
                   bool new_coverage, double coverage_rate,
                   std::size_t target_coverage_count, double target_best_score,
                   double target_score, double energy)
    : is_success(is_success), stdout_buffer(stdout_buffer),
      stderr_buffer(stderr_buffer), new_coverage(new_coverage),
      coverage_rate(coverage_rate),
      target_coverage_count(target_coverage_count),
      target_best_score(target_best_score), target_score(target_score),
      energy(energy) {}

std::string Feedback::get_stdout_str() const {
    return std::string(stdout_buffer.begin(), stdout_buffer.end());
}

std::string Feedback::get_stderr_str() const {
    return std::string(stderr_buffer.begin(), stderr_buffer.end());
}

std::string Feedback::to_string() const {
    std::string str;
    str += "[is_success]: " + std::to_string(is_success) + "\n";
    str += "[new_coverage]: " + std::to_string(new_coverage) + "\n";
    str += "[coverage_rate]: " + std::to_string(coverage_rate) + " ";
    str += "[target_coverage_count]: " + std::to_string(target_coverage_count) +
           "\n";
    str += "[target_best_score]: " + std::to_string(target_best_score) + " ";
    str += "[target_score]: " + std::to_string(target_score) + "\n";
    str += "[energy]: " + std::to_string(energy) + "\n";
    str += "[stdout_buffer]:\n";
    str += get_stdout_str() + "\n";
    str += "[stderr_buffer]:\n";
    str += get_stderr_str() + "\n";
    return str;
}

std::optional<std::size_t> Feedback::get_crash_line() const {
    if (is_success == false) {
        static const std::regex re(R"(:(\d+):\s+Assertion\s+failed)");
        std::smatch results;

        // Extract assertion line.
        std::string output = get_stdout_str();
        if (std::regex_search(output, results, re)) {
            std::cout << "assertion line: " << results[1].str() << std::endl;
            return std::stoi(results[1].str());
        } else {
            std::cout << "Failed to parse assertion line." << std::endl;
            return 0;
        }
    }
    return std::nullopt;
}
