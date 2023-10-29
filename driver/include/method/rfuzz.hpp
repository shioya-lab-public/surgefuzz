#pragma once

#include "util/util.hpp"
#include <cstdint>
#include <vector>

class RFuzz {
  public:
    RFuzz()
        : bitmap(get_shm_ptr<std::uint32_t>("FUZZ_BITMAP")),
          bitmap_byte_size(get_size_from_env("FUZZ_BITMAP_BYTE_SIZE")),
          chunk_count(bitmap_byte_size / 4), init_state(chunk_count) {}

    // Disable copy constructors.
    RFuzz(const RFuzz &) = delete;
    RFuzz &operator=(const RFuzz &) = delete;

    void update(auto *core) {
        static bool is_first = true;
        if (is_first) {
            initialize(core);
            is_first = false;
        }

        for (std::size_t chunk_idx = 0; chunk_idx < chunk_count; chunk_idx++) {
            bitmap[chunk_idx] |=
                init_state[chunk_idx] ^ core->coverage[chunk_idx];
        }
    }

  private:
    std::uint32_t *const bitmap;
    const std::size_t bitmap_byte_size;
    const std::size_t chunk_count;
    std::vector<std::uint32_t> init_state;

    void initialize(auto *core) {
        for (std::size_t chunk_idx = 0; chunk_idx < chunk_count; chunk_idx++) {
            init_state[chunk_idx] = core->coverage[chunk_idx];
        }
    }
};
