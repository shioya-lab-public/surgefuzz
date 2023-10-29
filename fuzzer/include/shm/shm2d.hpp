#pragma once

#include <cstring>
#include <iostream>
#include <sys/ipc.h> // for IPC_PRIVATE
#include <sys/shm.h> // for shmget, shmat, shmdt, and shmctl

template <typename T> class SHM2D {
  private:
    int shm_id;
    T *shm_addr;
    const std::size_t rows;
    const std::size_t cols;
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
        if (shm_addr == (T *)-1) {
            perror("Failed to attach to shared memory segment");
            shmctl(shm_id, IPC_RMID, nullptr);
            exit(EXIT_FAILURE);
        }

        set_env_shm_id();
    }

    void cleanup_shm() {
        reset_env_shm_id();

        if (shmdt(shm_addr) == -1) {
            perror("Failed to detach from shared memory segment");
        }
        if (shmctl(shm_id, IPC_RMID, nullptr) == -1) {
            perror("Failed to delete shared memory segment");
        }
    }

  public:
    SHM2D(std::size_t rows, std::size_t cols, const std::string &env_var_name)
        : rows(rows), cols(cols), env_var_name(env_var_name) {
        setup_shm(sizeof(T) * rows * cols);
    }

    ~SHM2D() { cleanup_shm(); }

    class Proxy {
      private:
        T *row_ptr;

      public:
        Proxy(T *ptr) : row_ptr(ptr) {}

        T &operator[](std::size_t col) { return row_ptr[col]; }
    };

    Proxy operator[](std::size_t row) const {
        if (row >= rows) {
            std::cerr << "Row index out of range" << std::endl;
            exit(EXIT_FAILURE);
        }
        return Proxy(&shm_addr[row * cols]);
    }
};
