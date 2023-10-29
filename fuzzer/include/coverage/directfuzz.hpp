#pragma once

#include "coverage/base.hpp"
#include "shm/shm2d.hpp"
#include "util.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

class DirectFuzz : public CoverageBase {
  public:
    DirectFuzz(const std::string &fuzz_bitmap_env_name)
        : bitmap_byte_size(get_size_from_env("FUZZ_BITMAP_BYTE_SIZE")),
          bitmap_instance_count(
              get_size_from_env("FUZZ_BITMAP_INSTANCE_COUNT")),
          chunks_per_instance(bitmap_byte_size / 4),
          max_coverage(get_size_from_env("FUZZ_MAX_COVERAGE")),
          bitmap(bitmap_instance_count,
                 std::vector<std::uint32_t>(chunks_per_instance, 0)),
          local_bitmap(bitmap_instance_count, chunks_per_instance,
                       fuzz_bitmap_env_name),
          instance_dist(get_vector_from_env("FUZZ_DIRECTFUZZ_INSTANCE_DIST")),
          max_energy(25.), min_energy(0.) {}

    // Disable copy constructors.
    // This class has the reference to raw shared memories,
    // so the instance should be at most only one.
    DirectFuzz(const DirectFuzz &) = delete;
    DirectFuzz &operator=(const DirectFuzz &) = delete;

    double coverage_rate() const override {
        std::size_t cnt = 0;
        for (std::size_t i = 0; i < bitmap_instance_count; i++) {
            for (std::size_t j = 0; j < chunks_per_instance; j++) {
                cnt += __builtin_popcount(bitmap[i][j]);
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
            for (std::size_t j = 0; j < chunks_per_instance; j++) {
                if (bitmap[i][j] != (bitmap[i][j] | local_bitmap[i][j])) {
                    is_new_coverage = true;
                    break;
                }
            }
        }
        return is_new_coverage;
    }

    void apply() override {
        for (std::size_t i = 0; i < bitmap_instance_count; i++) {
            for (std::size_t j = 0; j < chunks_per_instance; j++) {
                bitmap[i][j] |= local_bitmap[i][j];
            }
        }
    }

    double energy(uint64_t) const override {
        double dist = 0;
        std::uint64_t covered_mux_select_signal_num = 0;
        for (std::size_t i = 0; i < bitmap_instance_count; i++) {
            for (std::size_t j = 0; j < chunks_per_instance; j++) {
                int c = __builtin_popcount(bitmap[i][j]);
                covered_mux_select_signal_num += c;
                dist += c * instance_dist[i];
            }
        }

        if (covered_mux_select_signal_num == 0) {
            return min_energy;
        }

        dist = dist / covered_mux_select_signal_num;
        double energy = max_energy - ((max_energy - min_energy) * dist /
                                      get_instance_max_dist());
        return static_cast<std::uint64_t>(energy);
    }

  private:
    const std::size_t bitmap_byte_size;
    const std::size_t bitmap_instance_count;
    const std::size_t chunks_per_instance;
    const std::size_t max_coverage;
    std::vector<std::vector<std::uint32_t>> bitmap;
    SHM2D<std::uint32_t> local_bitmap;
    std::vector<std::size_t> instance_dist;
    const double max_energy;
    const double min_energy;

    std::uint64_t get_instance_max_dist() const {
        return *std::max_element(instance_dist.begin(), instance_dist.end());
    }
};
