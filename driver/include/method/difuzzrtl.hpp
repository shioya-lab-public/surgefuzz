#pragma once

#include "util/util.hpp"
#include <cstdint>

class DifuzzRTL {
  public:
    DifuzzRTL()
        : bitmap(get_shm_ptr<std::uint8_t>("FUZZ_BITMAP")),
          bitmap_byte_size(get_size_from_env("FUZZ_BITMAP_BYTE_SIZE")),
          bitmap_instance_count(
              get_size_from_env("FUZZ_BITMAP_INSTANCE_COUNT")) {}

    // Disable copy constructors.
    DifuzzRTL(const DifuzzRTL &) = delete;
    DifuzzRTL &operator=(const DifuzzRTL &) = delete;

    void update(auto *core) {
        for (std::size_t instance_idx = 0; instance_idx < bitmap_instance_count;
             instance_idx++) {
            bitmap[bitmap_byte_size * instance_idx +
                   core->coverage[instance_idx]] = 1;
        }
    }

  private:
    std::uint8_t *const bitmap;
    const std::size_t bitmap_byte_size;
    const std::size_t bitmap_instance_count;
};
