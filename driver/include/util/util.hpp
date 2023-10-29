#pragma once

#include <cstdlib> // for std::getenv, std::atoi, and std::strtoull
#include <iostream>
#include <sys/shm.h> // for shmat

// Helper function to retrieve and validate environment variable values.
inline const char *get_env_value(const char *env_var_name) {
    const char *env_value = std::getenv(env_var_name);
    if (!env_value) {
        std::cerr << "Environment variable " << env_var_name << " not set."
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    return env_value;
}

template <typename T> T *get_shm_ptr(const char *env_var_name) {
    const char *env_value = get_env_value(env_var_name);

    int shm_id = std::atoi(env_value);
    if (shm_id == 0 && env_value[0] != '0') { // atoi returns 0 on failure
        std::cerr << "Invalid integer value for environment variable "
                  << env_var_name << "." << std::endl;
        exit(EXIT_FAILURE);
    }

    T *shm_ptr = static_cast<T *>(shmat(shm_id, NULL, 0));
    if (shm_ptr == reinterpret_cast<T *>(-1)) {
        perror("shmat() failed");
        exit(EXIT_FAILURE);
    }

#ifdef DEBUG
    std::cout << "[Info] Shared Memory Details:" << std::endl;
    std::cout << "       - ID: " << shm_id << std::endl;
    std::cout << "       - Pointer Address: " << static_cast<void *>(shm_ptr)
              << std::endl;
#endif

    return shm_ptr;
}

inline std::size_t get_size_from_env(const char *env_var_name) {
    const char *env_value = get_env_value(env_var_name);
    return static_cast<std::size_t>(std::strtoull(env_value, nullptr, 10));
}
