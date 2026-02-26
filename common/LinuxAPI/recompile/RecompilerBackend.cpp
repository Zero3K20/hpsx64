/**
 * @file RecompilerBackend.cpp
 * @brief Linux/POSIX implementation of RecompilerBackend using mmap instead of VirtualAlloc
 */

#include "RecompilerBackend.h"
#include <algorithm>
#include <iostream>
#include <cstring>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

RecompilerBackend::CodeBlock::CodeBlock()
    : base(nullptr), current(nullptr), size(0), is_encoding(false) {
}

RecompilerBackend::RecompilerBackend(size_t block_size_power, size_t num_blocks)
    : num_blocks_(num_blocks), memory_base_(nullptr) {

    if (num_blocks == 0) {
        throw std::invalid_argument("Number of blocks must be greater than 0");
    }

    block_size_ = (block_size_power >= 12) ? (size_t(1) << block_size_power) : PAGE_SIZE;

    size_t total_size = block_size_ * num_blocks_;

    // Allocate executable memory using mmap
    memory_base_ = static_cast<uint8_t*>(mmap(
        nullptr,
        total_size,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    ));

    if (memory_base_ == MAP_FAILED) {
        memory_base_ = nullptr;
        throw std::runtime_error("Failed to allocate executable memory via mmap");
    }

    // Try to lock pages in RAM (best effort - may fail without CAP_IPC_LOCK)
    if (mlock(memory_base_, total_size) != 0) {
        std::cout << "Warning: Failed to lock executable memory pages (mlock failed)\n";
    }

    code_blocks_.resize(num_blocks_);
    for (size_t i = 0; i < num_blocks_; ++i) {
        code_blocks_[i].base = memory_base_ + (i * block_size_);
        code_blocks_[i].current = code_blocks_[i].base;
        code_blocks_[i].size = block_size_;
        code_blocks_[i].is_encoding = false;
    }
}

RecompilerBackend::~RecompilerBackend() {
    if (memory_base_) {
        size_t total_size = block_size_ * num_blocks_;
        munlock(memory_base_, total_size);
        munmap(memory_base_, total_size);
        memory_base_ = nullptr;
    }
}

RecompilerBackend::RecompilerBackend(RecompilerBackend&& other) noexcept
    : code_blocks_(std::move(other.code_blocks_))
    , block_size_(other.block_size_)
    , num_blocks_(other.num_blocks_)
    , memory_base_(other.memory_base_) {
    other.memory_base_ = nullptr;
    other.num_blocks_ = 0;
}

RecompilerBackend& RecompilerBackend::operator=(RecompilerBackend&& other) noexcept {
    if (this != &other) {
        if (memory_base_) {
            munmap(memory_base_, block_size_ * num_blocks_);
        }
        code_blocks_ = std::move(other.code_blocks_);
        block_size_ = other.block_size_;
        num_blocks_ = other.num_blocks_;
        memory_base_ = other.memory_base_;
        other.memory_base_ = nullptr;
        other.num_blocks_ = 0;
    }
    return *this;
}

uint8_t* RecompilerBackend::StartEncoding(size_t block_index) {
    if (block_index >= num_blocks_)
        throw std::out_of_range("Block index out of range");
    if (code_blocks_[block_index].is_encoding)
        throw std::runtime_error("Block is already being encoded");

    code_blocks_[block_index].is_encoding = true;
    code_blocks_[block_index].current = code_blocks_[block_index].base;
    // Fill with INT3 breakpoints for safety
    memset(code_blocks_[block_index].base, 0xCC, block_size_);
    return code_blocks_[block_index].base;
}

uint8_t* RecompilerBackend::EndEncoding(size_t block_index) {
    if (block_index >= num_blocks_)
        throw std::out_of_range("Block index out of range");
    if (!code_blocks_[block_index].is_encoding)
        throw std::runtime_error("Block is not currently being encoded");

    code_blocks_[block_index].is_encoding = false;

    // Flush instruction cache for the modified region
    __builtin___clear_cache(
        (char*)code_blocks_[block_index].base,
        (char*)code_blocks_[block_index].current
    );

    return code_blocks_[block_index].current;
}

void RecompilerBackend::Execute(size_t block_index) {
    if (block_index >= num_blocks_)
        throw std::out_of_range("Block index out of range");
    if (code_blocks_[block_index].is_encoding)
        throw std::runtime_error("Cannot execute a block that is being encoded");

    typedef void(*VoidFunc)();
    VoidFunc func = reinterpret_cast<VoidFunc>(code_blocks_[block_index].base);
    func();
}

uint8_t* RecompilerBackend::GetBlockBase(size_t block_index) {
    if (block_index >= num_blocks_)
        throw std::out_of_range("Block index out of range");
    return code_blocks_[block_index].base;
}

void RecompilerBackend::SetEncodingPosition(size_t block_index, uint8_t* position) {
    if (block_index >= num_blocks_)
        throw std::out_of_range("Block index out of range");
    if (!code_blocks_[block_index].is_encoding)
        throw std::runtime_error("Block is not currently being encoded");

    uint8_t* base = code_blocks_[block_index].base;
    if (position < base || position > base + block_size_)
        throw std::out_of_range("Position is outside block boundaries");

    code_blocks_[block_index].current = position;
}

size_t RecompilerBackend::GetRemainingSpace(size_t block_index) const {
    if (block_index >= num_blocks_)
        throw std::out_of_range("Block index out of range");
    if (!code_blocks_[block_index].is_encoding)
        return 0;
    return block_size_ - (code_blocks_[block_index].current - code_blocks_[block_index].base);
}

size_t RecompilerBackend::GetBlockSize() const { return block_size_; }
size_t RecompilerBackend::GetNumBlocks() const { return num_blocks_; }

bool RecompilerBackend::IsEncoding(size_t block_index) const {
    if (block_index >= num_blocks_)
        throw std::out_of_range("Block index out of range");
    return code_blocks_[block_index].is_encoding;
}

bool RecompilerBackend::IsMemoryLocked() const { return memory_base_ != nullptr; }
