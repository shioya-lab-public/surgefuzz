#include "util.hpp"
#include <iostream>
#include <sys/shm.h>

ShmInfo setup_shm(std::size_t size) {
    ShmInfo info = {-1, nullptr};

    int shm_id = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("Failed to allocate shared memory segment");
        return info;
    }

    void *shm_addr = shmat(shm_id, nullptr, 0);
    if (shm_addr == (void *)-1) {
        perror("Failed to allocate shared memory segment");
        shmctl(
            shm_id, IPC_RMID,
            nullptr); // If attachment fails, immediately release the resource
        return info;
    }

    info.shm_id = shm_id;
    info.shm_addr = shm_addr;

    return info;
}

void cleanup_shm(const ShmInfo &info) {
    if (shmdt(info.shm_addr) == -1) {
        perror("Failed to detach from shared memory segment");
    }

    if (shmctl(info.shm_id, IPC_RMID, nullptr) == -1) {
        perror("Failed to delete shared memory segment");
    }
}
