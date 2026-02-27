#pragma once
#include <cstddef> // for size_t
#include <cstdint> // for uint32_t, uint64_t
#include <vector>


class MemoryManager
{
public:
    // Prevent instantiation
    MemoryManager() = delete;
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    /**
     * @brief Maps the same physical memory to multiple user-specified virtual addresses.
     *
     * This function creates a pagefile-backed section and maps it into multiple
     * virtual addresses provided by the caller. All views point to the same offset
     * in the section and share the same physical memory.
     *
     * @param targetAddresses A vector of virtual addresses where each view should be mapped.
     *                        These must be page-aligned and reserved beforehand.
     * @param viewSize Size of the shared memory region (in bytes).
     * @param protection Memory protection flags (e.g., PAGE_READWRITE).
     * @return A vector of pointers to successfully mapped views. If any mapping fails, returns partial or empty vector.
     *
     * @note Use VirtualAlloc with MEM_RESERVE to reserve each target address before calling this.
     *       Use UnmapViewOfFile to release each view.
     */
    static std::vector<void*> MapSharedMemoryToSpecifiedAddresses(size_t numberOfAddresses, const uint64_t* targetAddresses, size_t viewSize, const uint32_t* protections);


};
