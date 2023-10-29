#pragma once

#include "shm/shm.hpp"
#include "util.hpp"
#include <cstdint>
#include <vector>

class Surge {
  public:
    Surge(const std::string &fuzz_bitmap_env_name)
        : bitmap_size(256), bitmap(bitmap_size, 0),
          local_bitmap(bitmap_size, fuzz_bitmap_env_name) {}

    // Disable copy constructors.
    // This class has the reference to raw shared memories,
    // so the instance should be at most only one.
    Surge(const Surge &) = delete;
    Surge &operator=(const Surge &) = delete;

    std::size_t coverage_count() const {
        std::size_t cnt = 0;
        for (std::size_t i = 0; i < bitmap_size; i++) {
            if (bitmap[i]) {
                cnt++;
            }
        }
        return cnt;
    }

    void reset() {
        for (std::size_t i = 0; i < bitmap_size; i++) {
            local_bitmap[i] = 0;
        }
    }

    std::uint64_t score() const { return get_max_value(local_bitmap); }

    std::uint64_t best_score() const { return get_max_value(bitmap); }

    void apply() {
        for (std::size_t i = 0; i < bitmap_size; i++) {
            if (!bitmap[i] && local_bitmap[i]) {
                bitmap[i] = local_bitmap[i];
            }
        }
    }

  private:
    const std::size_t bitmap_size;
    std::vector<std::uint8_t> bitmap;
    SHM<std::uint8_t> local_bitmap;

    template <typename Container>
    std::uint64_t get_max_value(const Container &data) const {
        std::uint64_t max_value = 0;
        for (std::size_t i = 0; i < bitmap_size; i++) {
            if (data[i]) {
                max_value = std::max<std::uint64_t>(max_value, i);
            }
        }
        return max_value;
    }
};
