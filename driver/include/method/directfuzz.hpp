#pragma once

#include "util/util.hpp"
#include <cstdint>
#include <vector>

class DirectFuzz {
  public:
    DirectFuzz()
        : bitmap(get_shm_ptr<std::uint32_t>("FUZZ_BITMAP")),
          bitmap_byte_size(get_size_from_env("FUZZ_BITMAP_BYTE_SIZE")),
          bitmap_instance_count(
              get_size_from_env("FUZZ_BITMAP_INSTANCE_COUNT")),
          chunks_per_instance(bitmap_byte_size / 4),
          init_state(bitmap_instance_count * chunks_per_instance) {}

    // Disable copy constructors.
    DirectFuzz(const DirectFuzz &) = delete;
    DirectFuzz &operator=(const DirectFuzz &) = delete;

    void update(auto *core) {
        static bool is_first = true;
        if (is_first) {
            initialize(core);
            is_first = false;
        }

        for (unsigned int instance_idx = 0;
             instance_idx < bitmap_instance_count; instance_idx++) {
            for (unsigned int chunk_idx = 0; chunk_idx < chunks_per_instance;
                 chunk_idx++) {
                bitmap[instance_idx * chunks_per_instance + chunk_idx] |=
                    init_state[instance_idx * chunks_per_instance + chunk_idx] ^
                    core->coverage[instance_idx][chunk_idx];
            }
        }
    }

  private:
    std::uint32_t *const bitmap;
    const std::size_t bitmap_byte_size;
    const std::size_t bitmap_instance_count;
    const std::size_t chunks_per_instance;
    std::vector<std::uint32_t> init_state;

    void initialize(auto *core) {
        for (unsigned int instance_idx = 0;
             instance_idx < bitmap_instance_count; instance_idx++) {
            for (unsigned int chunk_idx = 0; chunk_idx < chunks_per_instance;
                 chunk_idx++) {
                init_state[instance_idx * chunks_per_instance + chunk_idx] =
                    core->coverage[instance_idx][chunk_idx];
            }
        }
    }
};
