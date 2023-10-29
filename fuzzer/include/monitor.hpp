#pragma once

#include "coverage/base.hpp"
#include "surge/surge.hpp"
#include <type_traits>

template <typename T>
concept ConceptCoverage = std::is_base_of_v<CoverageBase, T>;

template <ConceptCoverage T> class Monitor {
  public:
    Monitor() : coverage(ENV_COV_NAME), surge(ENV_TARGET_COV_NAME){};

    // Disable copy constructors.
    // This class has the reference to raw shared memories,
    // so the instance should be at most only one.
    Monitor(const Monitor &) = delete;
    Monitor &operator=(const Monitor &) = delete;

    void reset() {
        coverage.reset();
        surge.reset();
    }

    void update() {
        coverage.apply();
        surge.apply();
    }

    bool feedback_new_coverage() const { return coverage.new_coverage(); }

    double feedback_coverage_rate() const { return coverage.coverage_rate(); }

    double feedback_energy() const { return coverage.energy(surge.score()); }

    std::size_t feedback_target_coverage_count() const {
        return surge.coverage_count();
    }

    double feedback_target_score() const { return surge.score(); }

    double feedback_target_best_score() const { return surge.best_score(); }

    inline static const std::string ENV_COV_NAME = "FUZZ_BITMAP";
    inline static const std::string ENV_TARGET_COV_NAME =
        "FUZZ_SURGE_SCORE_BITMAP";

  private:
    T coverage;
    Surge surge;
};
