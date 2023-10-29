#pragma once

#include "coverage/base.hpp"
#include "shm/shm2d.hpp"
#include "util.hpp"
#include <cstdint>
#include <vector>

class DifuzzRTL : public CoverageBase {
  public:
    DifuzzRTL(const std::string &fuzz_bitmap_env_name)
        : bitmap_byte_size(get_size_from_env("FUZZ_BITMAP_BYTE_SIZE")),
          bitmap_instance_count(
              get_size_from_env("FUZZ_BITMAP_INSTANCE_COUNT")),
          max_coverage(get_size_from_env("FUZZ_MAX_COVERAGE")),
          bitmap(bitmap_instance_count,
                 std::vector<std::uint8_t>(bitmap_byte_size, 0)),
          local_bitmap(bitmap_instance_count, bitmap_byte_size,
                       fuzz_bitmap_env_name) {}

    // Disable copy constructors.
    // This class has the reference to raw shared memories,
    // so the instance should be at most only one.
    DifuzzRTL(const DifuzzRTL &) = delete;
    DifuzzRTL &operator=(const DifuzzRTL &) = delete;

    double coverage_rate() const override {
        std::size_t cnt = 0;
        for (std::size_t i = 0; i < bitmap_instance_count; i++) {
            for (std::size_t j = 0; j < bitmap_byte_size; j++) {
                if (bitmap[i][j]) {
                    cnt++;
                }
            }
        }
        return 100. * cnt / max_coverage;
    }

    void reset() override {
        for (std::size_t i = 0; i < bitmap_instance_count; i++) {
            for (std::size_t j = 0; j < bitmap_byte_size; j++) {
                local_bitmap[i][j] = 0;
            }
        }
    }

    bool new_coverage() const override {
        bool is_new_coverage = false;
        for (std::size_t i = 0; i < bitmap_instance_count; i++) {
            for (std::size_t j = 0; j < bitmap_byte_size; j++) {
                if (!bitmap[i][j] && local_bitmap[i][j]) {
                    is_new_coverage = true;
                    break;
                }
            }
        }
        return is_new_coverage;
    }

    void apply() {
        for (std::size_t i = 0; i < bitmap_instance_count; i++) {
            for (std::size_t j = 0; j < bitmap_byte_size; j++) {
                if (!bitmap[i][j] && local_bitmap[i][j]) {
                    bitmap[i][j] = local_bitmap[i][j];
                }
            }
        }
    }

  private:
    const std::size_t bitmap_byte_size;
    const std::size_t bitmap_instance_count;
    const std::size_t max_coverage;
    std::vector<std::vector<std::uint8_t>> bitmap;
    SHM2D<std::uint8_t> local_bitmap;
};
