#pragma once

#include "coverage/base.hpp"
#include "shm/shm.hpp"
#include "util.hpp"
#include <cstdint>
#include <vector>

class SurgeFuzz : public CoverageBase {
  public:
    SurgeFuzz(const std::string &fuzz_bitmap_env_name)
        : bitmap_byte_size(get_size_from_env("FUZZ_BITMAP_BYTE_SIZE")),
          max_coverage(get_size_from_env("FUZZ_MAX_COVERAGE")),
          bitmap(bitmap_byte_size, 0),
          local_bitmap(bitmap_byte_size, fuzz_bitmap_env_name) {}

    // Disable copy constructors.
    // This class has the reference to raw shared memories,
    // so the instance should be at most only one.
    SurgeFuzz(const SurgeFuzz &) = delete;
    SurgeFuzz &operator=(const SurgeFuzz &) = delete;

    double coverage_rate() const override {
        std::size_t cnt = 0;
        for (std::size_t i = 0; i < bitmap_byte_size; i++) {
            if (bitmap[i]) {
                cnt++;
            }
        }
        return 100. * cnt / max_coverage;
    }

    void reset() override {
        for (std::size_t i = 0; i < bitmap_byte_size; i++) {
            local_bitmap[i] = 0;
        }
    }

    bool new_coverage() const override {
        bool is_new_coverage = false;
        for (std::size_t i = 0; i < bitmap_byte_size; i++) {
            if (!bitmap[i] && local_bitmap[i]) {
                is_new_coverage = true;
                break;
            }
        }
        return is_new_coverage;
    }

    void apply() override {
        for (std::size_t i = 0; i < bitmap_byte_size; i++) {
            if (!bitmap[i] && local_bitmap[i]) {
                bitmap[i] = local_bitmap[i];
            }
        }
    }

#if defined(ENABLE_POWER_SCHEDULE)
    double energy(std::uint64_t surge_score) const {
        return surge_score * surge_score;
    }
#endif

  private:
    std::size_t bitmap_byte_size;
    std::size_t max_coverage;
    std::vector<std::uint8_t> bitmap;
    SHM<std::uint8_t> local_bitmap;
};
