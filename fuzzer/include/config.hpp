#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>

class Config {
  private:
    Config()
        : initial_seed_count(1), initial_seed_block_count(4),
          initial_seed_instructions_per_block(5), enable_rv64a(false),
          enable_rv64im(false), enable_insert_memory_access_sequence(true),
          max_operation_count(3) {}

  public:
    // For initial seed generation.
    std::size_t initial_seed_count; // The number of initial seeds automatically
                                    // generated at the start of the process.
    int initial_seed_block_count;   // The number of instructions in each block
                                    // for the initial seeds.
    int initial_seed_instructions_per_block; // The total number of blocks in an
                                             // initial seed.

    // For instruction generation.
    bool enable_rv64a;
    bool enable_rv64im;
    bool enable_insert_memory_access_sequence;
    int max_operation_count; // The maximum number of operations that can be
                             // applied during a single mutation process.

    static Config &instance() {
        static Config instance;
        return instance;
    }

    void parse_command_line_args(int argc, char *argv[]) {
        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "--initial-seed-count") == 0 && i + 1 < argc) {
                initial_seed_count = std::stoi(argv[++i]);
            } else if (strcmp(argv[i], "--initial-seed-block-count") == 0 &&
                       i + 1 < argc) {
                initial_seed_block_count = std::stoi(argv[++i]);
            } else if (strcmp(argv[i],
                              "--initial-seed-instructions-per-block") == 0 &&
                       i + 1 < argc) {
                initial_seed_instructions_per_block = std::stoi(argv[++i]);
            } else if (strcmp(argv[i], "--enable-rv64a") == 0) {
                enable_rv64a = true;
            } else if (strcmp(argv[i], "--disable-rv64a") == 0) {
                enable_rv64a = false;
            } else if (strcmp(argv[i], "--enable-rv64im") == 0) {
                enable_rv64im = true;
            } else if (strcmp(argv[i], "--disable-rv64im") == 0) {
                enable_rv64im = false;
            } else if (strcmp(argv[i],
                              "--enable-insert-memory-access-sequence") == 0) {
                enable_insert_memory_access_sequence = true;
            } else if (strcmp(argv[i],
                              "--disable-insert-memory-access-sequence") == 0) {
                enable_insert_memory_access_sequence = false;
            } else if (strcmp(argv[i], "--max-operation-count") == 0 &&
                       i + 1 < argc) {
                max_operation_count = std::stoi(argv[++i]);
            }
        }
    }
};
