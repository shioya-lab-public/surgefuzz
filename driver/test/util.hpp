#pragma once

#include <iostream>
#include <sys/shm.h>

struct ShmInfo {
    int shm_id;
    void *shm_addr;
};

ShmInfo setup_shm(std::size_t size);
void cleanup_shm(const ShmInfo &info);
