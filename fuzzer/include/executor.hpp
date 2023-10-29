#pragma once

#include "common.hpp"
#include "feedback.hpp"
#include "monitor.hpp"
#include <array>
#include <filesystem>
#include <string>
#include <vector>

template <typename T> class Executor {
  public:
    Executor(const std::vector<std::string> &argv, bool enable_record_output);

    // Disable copy constructors.
    Executor(const Executor &) = delete;
    Executor(Executor &&) = delete;
    Executor &operator=(const Executor &) = delete;
    Executor &operator=(Executor &&) = delete;
    Executor() = delete;

    Feedback run();

  private:
    const std::vector<std::string> argv;
    const std::uint32_t time_limit_ms;
    const bool enable_record_output;

    std::vector<const char *> cargv;
    int null_fd;

    std::vector<std::uint8_t> stdout_buffer;
    std::vector<std::uint8_t> stderr_buffer;

    Monitor<T> monitor;

    bool read_chunk(std::vector<std::uint8_t> &dest, int fd);
};

class Pipe {
  public:
    static const int READ = 0;
    static const int WRITE = 1;
    static const int INVALID_FD = -1;

    Pipe();

    void setup();
    void close();
    void close_read_fd();
    void close_write_fd();
    int read_fd() const;
    int write_fd() const;

  private:
    std::array<int, 2> fd;
};
