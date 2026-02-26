/*
 * memmgmt.cpp - Linux/POSIX implementation of MemoryManager
 * Maps the same shared memory to multiple virtual addresses using memfd + mmap
 */

#include <iostream>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

// Use memfd_create for anonymous shared memory
#if defined(__linux__)
#include <sys/syscall.h>
#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif
static int linux_memfd_create(const char* name, unsigned int flags) {
    return (int)syscall(SYS_memfd_create, name, flags);
}
#else
#include <fcntl.h>
#include <sys/stat.h>
#endif

#include "memmgmt.h"

std::vector<void*> MemoryManager::MapSharedMemoryToSpecifiedAddresses(
    size_t numberOfAddresses,
    const uint64_t* targetAddresses,
    size_t viewSize,
    const uint32_t* protections)
{
    std::vector<void*> views;

    if (numberOfAddresses == 0 || viewSize == 0) {
        return views;
    }

    // Round up viewSize to page boundary
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (page_size <= 0) page_size = 4096;
    size_t aligned_size = ((viewSize + page_size - 1) / page_size) * page_size;

#if defined(__linux__)
    // Create anonymous shared memory file descriptor
    int fd = linux_memfd_create("hpsx64_shmem", MFD_CLOEXEC);
    if (fd < 0) {
        std::cerr << "memfd_create failed: " << strerror(errno) << "\n";
        return views;
    }
#else
    // Fallback for other POSIX systems: use shm_open
    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "/hpsx64_shmem_%d", (int)getpid());
    int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0600);
    if (fd < 0) {
        std::cerr << "shm_open failed: " << strerror(errno) << "\n";
        return views;
    }
    shm_unlink(shm_name); // unlink immediately so it's auto-cleaned
#endif

    // Set size
    if (ftruncate(fd, (off_t)aligned_size) != 0) {
        std::cerr << "ftruncate failed: " << strerror(errno) << "\n";
        close(fd);
        return views;
    }

    // Map to each specified address
    for (size_t i = 0; i < numberOfAddresses; i++) {
        void* addr = (void*)(uintptr_t)(targetAddresses[i] & 0x7FFFFFFFFFFFFFFFllu);
        int prot = (int)(protections[i]);
        // Ensure prot is valid for mmap
        if (prot == 0) prot = PROT_READ | PROT_WRITE;

        void* view = mmap(addr, aligned_size, prot,
                          MAP_SHARED | (addr ? MAP_FIXED_NOREPLACE : 0), fd, 0);
        if (view == MAP_FAILED) {
            // Try without MAP_FIXED_NOREPLACE (older kernels)
            view = mmap(addr, aligned_size, prot,
                        MAP_SHARED | (addr ? MAP_FIXED : 0), fd, 0);
        }
        if (view == MAP_FAILED) {
            std::cerr << "mmap failed at address 0x" << std::hex << (uintptr_t)addr
                      << ": " << strerror(errno) << "\n";
            // Continue - some addresses may not be mappable
            views.push_back(nullptr);
            continue;
        }
        views.push_back(view);
    }

    close(fd);
    return views;
}
