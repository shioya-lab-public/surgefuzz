#include "common.hpp"
#include "config.hpp"
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>

std::string num2str(int num, int len) {
    std::stringstream ss;
    ss << std::setw(len) << std::setfill('0') << num;
    return ss.str();
}

RandomNumberGenerator::RandomNumberGenerator()
    : mt(std::chrono::steady_clock::now().time_since_epoch().count()) {}

RandomNumberGenerator::RandomNumberGenerator(int seed) : mt(seed) {}

int RandomNumberGenerator::operator()(int a, int b) { // [a, b)
    std::uniform_int_distribution<int> dist(a, b - 1);
    return dist(mt);
}

int RandomNumberGenerator::operator()(int b) { // [0, b)
    return (*this)(0, b);
}

int execute_command(const std::vector<std::string> &argv,
                    bool redirect_output) {
    if (argv.size() == 0) {
        std::cerr << "Too short arguments." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork() failed");
        std::exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // If redirect_output is true, redirect the child's stdout to /dev/null
        if (redirect_output) {
            // Open /dev/null for writing
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull == -1) {
                perror("open(/dev/null) failed");
                std::exit(EXIT_FAILURE);
            }

            // Redirect stdout to /dev/null
            if (dup2(devnull, STDOUT_FILENO) == -1) {
                perror("dup2() failed");
                std::exit(EXIT_FAILURE);
            }

            close(devnull); // We no longer need the /dev/null fd
        }

        // Setup arguments
        std::vector<const char *> cargv;
        for (const auto &v : argv) {
            cargv.emplace_back(v.c_str());
        }
        cargv.emplace_back(nullptr);

        std::string cmd = "";
        for (const auto &arg : argv) {
            cmd += arg + " ";
        }
#ifdef ENABLE_DEBUG_PRINT
        std::cout << "Run command: " << cmd << std::endl;
#endif

        // Execute a command in the child process.
        if (execvp(cargv[0], (char **)cargv.data()) < 0) {
            perror("execvp() failed");
            std::exit(EXIT_FAILURE);
        }
        __builtin_unreachable();
    } else {
        int wstatus;
        if (waitpid(pid, &wstatus, 0) < 0) {
            perror("waitpid() failed");
            std::exit(EXIT_FAILURE);
        }
        return WEXITSTATUS(wstatus);
    }
    __builtin_unreachable();
}
