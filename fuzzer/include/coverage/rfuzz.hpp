#pragma once

#include "coverage/base.hpp"
#include "shm/shm.hpp"
#include "util.hpp"
#include <cstdint>
#include <vector>

class RFuzz : public CoverageBase {
  public:
    RFuzz(const std::string &fuzz_bitmap_env_name)
        : bitmap_byte_size(get_size_from_env("FUZZ_BITMAP_BYTE_SIZE")),
          chunk_count(bitmap_byte_size / 4),
          max_coverage(get_size_from_env("FUZZ_MAX_COVERAGE")),
          bitmap(chunk_count, 0),
          local_bitmap(chunk_count, fuzz_bitmap_env_name) {}

    // Disable copy constructors.
    // This class has the reference to raw shared memories,
    // so the instance should be at most only one.
    RFuzz(const RFuzz &) = delete;
    RFuzz &operator=(const RFuzz &) = delete;

    double coverage_rate() const override {
        std::size_t cnt = 0;
        for (std::size_t i = 0; i < chunk_count; i++) {
            cnt += __builtin_popcount(bitmap[i]);
        }
        return 100. * cnt / max_coverage;
    }

    void reset() override {
        for (std::size_t i = 0; i < chunk_count; i++) {
            local_bitmap[i] = 0;
        }
    }

    bool new_coverage() const override {
        bool is_new_coverage = false;
        for (std::size_t i = 0; i < chunk_count; i++) {
            if (bitmap[i] != (bitmap[i] | local_bitmap[i])) {
                is_new_coverage = true;
                break;
            }
        }
        return is_new_coverage;
    }

    void apply() override {
        for (std::size_t i = 0; i < chunk_count; i++) {
            bitmap[i] |= local_bitmap[i];
        }
    }

  private:
    const std::size_t bitmap_byte_size;
    const std::size_t chunk_count;
    const std::size_t max_coverage;
    std::vector<std::uint32_t> bitmap;
    SHM<std::uint32_t> local_bitmap;
};
