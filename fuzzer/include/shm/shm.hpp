#pragma once

#include <cstring>
#include <iostream>
#include <sys/ipc.h> // for IPC_PRIVATE
#include <sys/shm.h> // for shmget, shmat, shmdt, and shmctl

template <typename T> class SHM {
  private:
    int shm_id;
    void *shm_addr;
    const std::size_t count;
    const std::string env_var_name;

    void set_env_shm_id() {
        setenv(env_var_name.c_str(), std::to_string(shm_id).c_str(), 1);
    }

    void reset_env_shm_id() { unsetenv(env_var_name.c_str()); }

    void setup_shm(std::size_t size) {
        shm_id = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666);
        if (shm_id == -1) {
            perror("Failed to allocate shared memory segment");
            exit(EXIT_FAILURE);
        }

        shm_addr = static_cast<T *>(shmat(shm_id, nullptr, 0));
        if (shm_addr == (void *)-1) {
            perror("Failed to attach to shared memory segment");
            shmctl(shm_id, IPC_RMID,
                   nullptr); // If attachment fails, immediately release the
                             // resource
            exit(EXIT_FAILURE);
        }

        set_env_shm_id();
    }

    void cleanup_shm() {
        if (shmdt(shm_addr) == -1) {
            perror("Failed to detach from shared memory segment");
        }
        if (shmctl(shm_id, IPC_RMID, nullptr) == -1) {
            perror("Failed to delete shared memory segment");
        }
    }

  public:
    explicit SHM(std::size_t n, const std::string &env_var_name)
        : count(n), env_var_name(env_var_name) {
        setup_shm(sizeof(T) * count);
    }

    ~SHM() { cleanup_shm(); }

    T &operator[](std::size_t index) const {
        if (index >= count) {
            std::cerr << "Index out of range" << std::endl;
            exit(EXIT_FAILURE);
        }
        return static_cast<T *>(shm_addr)[index];
    }
};
