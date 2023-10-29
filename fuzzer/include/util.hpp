#pragma once

#include <map>
#include <string>
#include <vector>

// Helper function to retrieve and validate environment variable values.
const char *get_env_value(const char *env_var_name);

std::size_t get_size_from_env(const char *env_var_name);

std::vector<std::size_t> get_vector_from_env(const char *env_var_name);

// Parses the given file to extract environment variable names and their
// associated values.
std::map<std::string, std::string> parse_env_vars(const std::string &filename);

std::vector<std::string> split(const std::string &s, char delim = ' ');
