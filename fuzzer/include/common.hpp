#pragma once

#include <chrono>
#include <random>
#include <string>
#include <vector>

std::string num2str(int num, int len);

class RandomNumberGenerator {
  public:
    RandomNumberGenerator();
    RandomNumberGenerator(int seed);
    int operator()(int a, int b);
    int operator()(int b);
    std::mt19937 mt;
};

enum class Mode { SurgeFuzz, DifuzzRTL, DirectFuzz, RFuzz, Blackbox, Invalid };

int execute_command(const std::vector<std::string> &argv,
                    bool redirect_output = false);

enum class Result { Success, Error, Timeout };

template <class result_t = std::chrono::milliseconds,
          class clock_t = std::chrono::steady_clock,
          class duration_t = std::chrono::milliseconds>
auto since(std::chrono::time_point<clock_t, duration_t> const &start) {
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}
