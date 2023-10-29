#pragma once

#include "coverage.hpp"
#include "reg_state.hpp"

#include <cstring>
#include <string>
#include <sys/shm.h>

template <typename T> class ShmManager {
  public:
    static constexpr int INVALID_SHM_ID = -1;

    ShmManager(const std::string &env_var_name)
        : shm_id(INVALID_SHM_ID), shm_ptr(nullptr), env_var_name(env_var_name) {
    }

    ~ShmManager() { remove(); }

    // Disable copy constructor.
    // This class has the reference to raw shared memories,
    // so the instance should be at most only one.
    ShmManager(const ShmManager &) = delete;
    ShmManager &operator=(const ShmManager &) = delete;

    void setup();
    void reset();
    void remove();
    T *get_shm_ptr() const;

  private:
    int shm_id;
    T *shm_ptr;
    std::string env_var_name;

    void allocate_shm();
    void free_shm();
    void attach_shm();
    void deattach_shm();
    void set_env_var();
    void unset_env_var();
};
