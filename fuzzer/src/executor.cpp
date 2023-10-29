#include "executor.hpp"
#include "coverage/blackbox.hpp"
#include "coverage/difuzzrtl.hpp"
#include "coverage/directfuzz.hpp"
#include "coverage/rfuzz.hpp"
#include "coverage/surgefuzz.hpp"
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

template <class T>
Executor<T>::Executor(const std::vector<std::string> &argv,
                      bool enable_record_output)
    : argv(argv), time_limit_ms(0), enable_record_output(enable_record_output),
      monitor() {
    // Setup arguments
    for (const auto &v : argv) {
        cargv.emplace_back(v.c_str());
    }
    cargv.emplace_back(nullptr);
    null_fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
}

template <class T> Feedback Executor<T>::run() {
    // Reset shared memory.
    monitor.reset();

    Pipe stdout_pipe;
    Pipe stderr_pipe;

    if (enable_record_output) {
        // These pipes will be used by parent process to tell child which file
        // to redirect to.
        stdout_pipe.setup();
        stderr_pipe.setup();
        stdout_buffer.clear();
        stderr_buffer.clear();
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        std::exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        if (enable_record_output) {
            // Redirect stdout/stderr to the pipe.
            dup2(stdout_pipe.write_fd(), STDOUT_FILENO);
            dup2(stderr_pipe.write_fd(), STDERR_FILENO);
            // Close unsed pipes.
            stdout_pipe.close();
            stderr_pipe.close();
        } else {
            // Redirect stdout/stderr to null.
            dup2(null_fd, STDOUT_FILENO);
            dup2(null_fd, STDERR_FILENO);
        }

        // Execute a command in the child process.
        if (execvp(cargv[0], (char **)cargv.data()) < 0) {
            perror("execvp() failed");
            std::exit(EXIT_FAILURE);
        }
        __builtin_unreachable();
    } else {
        if (enable_record_output) {
            stdout_pipe.close_write_fd();
            stderr_pipe.close_write_fd();
        }

        // Wait for the child process
        int put_status;
        if (waitpid(pid, &put_status, 0) < 0) {
            perror("waitpid() failed");
            std::exit(EXIT_FAILURE);
        }

        if (enable_record_output) {
            while (1) {
                int readed_size =
                    read_chunk(stdout_buffer, stdout_pipe.read_fd());
                if (readed_size == 0) {
                    break;
                }
            }
            while (1) {
                int readed_size =
                    read_chunk(stderr_buffer, stderr_pipe.read_fd());
                if (readed_size == 0) {
                    break;
                }
            }
            stdout_pipe.close_read_fd();
            stderr_pipe.close_read_fd();
        }

        const bool is_success = put_status == 0;

        // Check before global bitmap is rewritten.
        bool new_coverage = monitor.feedback_new_coverage();

        // Update global bitmap using shared memory.
        monitor.update();

        return Feedback(is_success, stdout_buffer, stderr_buffer, new_coverage,
                        monitor.feedback_coverage_rate(),
                        monitor.feedback_target_coverage_count(),
                        monitor.feedback_target_best_score(),
                        monitor.feedback_target_score(),
                        monitor.feedback_energy());
    }
}

template <class T>
bool Executor<T>::read_chunk(std::vector<std::uint8_t> &dest, int fd) {
    static const std::size_t block_size = 1024;
    std::size_t cur_size = dest.size();
    dest.resize(cur_size + block_size);
    auto readed_size = read(fd, std::next(dest.data(), cur_size), block_size);
    if (readed_size == -1) {
        perror("read() failed");
        std::exit(EXIT_FAILURE);
    }
    dest.resize(cur_size + readed_size);
    return readed_size != 0;
}

template class Executor<SurgeFuzz>;
template class Executor<RFuzz>;
template class Executor<DifuzzRTL>;
template class Executor<DirectFuzz>;
template class Executor<Blackbox>;

Pipe::Pipe() : fd({INVALID_FD, INVALID_FD}){};

void Pipe::setup() {
    if (pipe(fd.data())) {
        perror("pipe() failed");
        std::exit(EXIT_FAILURE);
    }
}

void Pipe::close() {
    close_read_fd();
    close_write_fd();
}

void Pipe::close_read_fd() {
    if (fd[READ] == INVALID_FD) {
        return;
    }
    if (::close(fd[READ]) == -1) {
        perror("close() failed");
    }
    fd[READ] = INVALID_FD;
}

void Pipe::close_write_fd() {
    if (fd[WRITE] == INVALID_FD) {
        return;
    }
    if (::close(fd[WRITE]) == -1) {
        perror("close() failed");
    }
    fd[WRITE] = INVALID_FD;
}

int Pipe::read_fd() const { return fd[READ]; }
int Pipe::write_fd() const { return fd[WRITE]; }
