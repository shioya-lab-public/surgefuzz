#pragma once

#include "coverage/base.hpp"
#include <cstdint>
#include <cstring>

class Blackbox : public CoverageBase {
  public:
    Blackbox(const std::string &) {}

    // Disable copy constructors.
    // This class has the reference to raw shared memories,
    // so the instance should be at most only one.
    Blackbox(const Blackbox &) = delete;
    Blackbox &operator=(const Blackbox &) = delete;

    double coverage_rate() const override { return 0.; }
    void reset() override {}
    bool new_coverage() const override { return false; }
    void apply() override {}
};
