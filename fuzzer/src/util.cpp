#include "util.hpp"
#include <cstdlib> // for std::getenv, std::atoi, and std::strtoull
#include <fstream>
#include <iostream>
#include <sstream>

// Helper function to retrieve and validate environment variable values.
const char *get_env_value(const char *env_var_name) {
    const char *env_value = std::getenv(env_var_name);
    if (!env_value) {
        std::cerr << "Environment variable " << env_var_name << " not set."
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    return env_value;
}

std::size_t get_size_from_env(const char *env_var_name) {
    const char *env_value = get_env_value(env_var_name);
    return static_cast<std::size_t>(std::strtoull(env_value, nullptr, 10));
}

std::vector<std::size_t> get_vector_from_env(const char *env_var_name) {
    const char *env_value = get_env_value(env_var_name);

    std::string value_str(env_value);
    std::size_t pos = 0;
    std::size_t comma_pos = value_str.find(",");

    std::vector<std::size_t> result;
    while (comma_pos != std::string::npos) {
        std::string token = value_str.substr(pos, comma_pos - pos);
        result.emplace_back(static_cast<std::size_t>(
            std::strtoull(token.c_str(), nullptr, 10)));

        pos = comma_pos + 1;
        comma_pos = value_str.find(",", pos);
    }

    result.emplace_back(static_cast<std::size_t>(
        std::strtoull(value_str.substr(pos).c_str(), nullptr, 10)));

    return result;
}

std::map<std::string, std::string> parse_env_vars(const std::string &filename) {
    std::map<std::string, std::string> result;

    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return result;
    }

    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string key, value;

        // Read up to the ':' into key
        if (!std::getline(iss, key, ':')) {
            std::cerr << "Invalid format in line (missing ':'): " << line
                      << std::endl;
            continue;
        }

        // Read the remaining part into value
        std::getline(iss, value);

        // Trim spaces from the key and value
        key.erase(0, key.find_first_not_of(' '));
        key.erase(key.find_last_not_of(' ') + 1);

        value.erase(0, value.find_first_not_of(' '));
        value.erase(value.find_last_not_of(' ') + 1);

        result[key] = value;
    }

    return result;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::stringstream ss(s);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(ss, token, delim)) {
        tokens.emplace_back(token);
    }
    return tokens;
}
