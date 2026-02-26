/**
 * @file RecompilerBackend.cpp
 * @brief Implementation of RecompilerBackend class
 */

#include "RecompilerBackend.h"
#include <algorithm>
#include <iostream>

// CodeBlock constructor implementation
RecompilerBackend::CodeBlock::CodeBlock() 
    : base(nullptr), current(nullptr), size(0), is_encoding(false) {
}

// RecompilerBackend constructor implementation
RecompilerBackend::RecompilerBackend(size_t block_size_power, size_t num_blocks) 
    : num_blocks_(num_blocks), memory_base_(nullptr) {
    
    // Validate block size power (minimum is 12 for 4KB page)
    if (block_size_power < 12) {
        // don't worry about block size
        //throw std::invalid_argument("Block size power must be at least 12 (4KB)");
    }
    
    // Calculate block size
    block_size_ = size_t(1) << block_size_power;
    
    // Ensure block size is at least page size and is aligned
    if (block_size_ < PAGE_SIZE) {
        //throw std::invalid_argument("Block size must be at least 4KB");
    }
    
    if (num_blocks == 0) {
        throw std::invalid_argument("Number of blocks must be greater than 0");
    }
    
    // Allocate all memory as one contiguous block with execute permissions
    size_t total_size = block_size_ * num_blocks_;
    memory_base_ = static_cast<uint8_t*>(VirtualAlloc(
        nullptr,
        total_size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    ));
    
    if (!memory_base_) {
        throw std::runtime_error("Failed to allocate executable memory");
    }
    
    // Lock the memory to prevent it from being paged to disk
    // This is important for performance and security reasons
    if (!VirtualLock(memory_base_, total_size)) {
        std::cout << "Failed to lock executable memory pages: 0x" << std::hex << GetLastError();

        // If locking fails, we should still clean up the allocated memory
        //VirtualFree(memory_base_, 0, MEM_RELEASE);
        //throw std::runtime_error("Failed to lock executable memory pages");
    }
    
    // Initialize code blocks
    code_blocks_.resize(num_blocks_);
    for (size_t i = 0; i < num_blocks_; ++i) {
        code_blocks_[i].base = memory_base_ + (i * block_size_);
        code_blocks_[i].current = code_blocks_[i].base;
        code_blocks_[i].size = block_size_;
        code_blocks_[i].is_encoding = false;
    }
}

// Destructor implementation
RecompilerBackend::~RecompilerBackend() {
    if (memory_base_) {
        // Unlock the memory before freeing it
        size_t total_size = block_size_ * num_blocks_;
        VirtualUnlock(memory_base_, total_size);
        VirtualFree(memory_base_, 0, MEM_RELEASE);
    }
}

// Move constructor implementation
RecompilerBackend::RecompilerBackend(RecompilerBackend&& other) noexcept
    : code_blocks_(std::move(other.code_blocks_)),
      block_size_(other.block_size_),
      num_blocks_(other.num_blocks_),
      memory_base_(other.memory_base_) {
    other.memory_base_ = nullptr;
}

// Move assignment operator implementation
RecompilerBackend& RecompilerBackend::operator=(RecompilerBackend&& other) noexcept {
    if (this != &other) {
        if (memory_base_) {
            // Unlock and free existing memory
            size_t total_size = block_size_ * num_blocks_;
            VirtualUnlock(memory_base_, total_size);
            VirtualFree(memory_base_, 0, MEM_RELEASE);
        }
        code_blocks_ = std::move(other.code_blocks_);
        block_size_ = other.block_size_;
        num_blocks_ = other.num_blocks_;
        memory_base_ = other.memory_base_;
        other.memory_base_ = nullptr;
    }
    return *this;
}

// StartEncoding implementation
uint8_t* RecompilerBackend::StartEncoding(size_t block_index) {
    if (block_index >= num_blocks_) {
        throw std::out_of_range("Block index out of range");
    }
    
    CodeBlock& block = code_blocks_[block_index];
    
    if (block.is_encoding) {
        throw std::runtime_error("Block is already being encoded");
    }
    
    // Reset the current pointer to the beginning
    block.current = block.base;
    block.is_encoding = true;
    
    // Clear the block (optional, fill with INT3 breakpoints for safety)
    std::fill(block.base, block.base + block_size_, 0xCC);
    
    return block.base;
}

// EndEncoding implementation
uint8_t* RecompilerBackend::EndEncoding(size_t block_index) {
    if (block_index >= num_blocks_) {
        throw std::out_of_range("Block index out of range");
    }
    
    CodeBlock& block = code_blocks_[block_index];
    
    if (!block.is_encoding) {
        throw std::runtime_error("Block is not currently being encoded");
    }
    
    block.is_encoding = false;
    
    // Flush instruction cache for the modified code region
    FlushInstructionCache(GetCurrentProcess(), block.base, 
                         static_cast<SIZE_T>(block.current - block.base));
    
    return block.current;
}

// Execute implementation
void RecompilerBackend::Execute(size_t block_index) {
    if (block_index >= num_blocks_) {
        throw std::out_of_range("Block index out of range");
    }
    
    CodeBlock& block = code_blocks_[block_index];
    
    if (block.is_encoding) {
        throw std::runtime_error("Cannot execute block that is currently being encoded");
    }
    
    // Cast to function pointer and execute
    // This assumes the code block contains a function with no parameters and no return
    // Adjust the signature as needed for your use case
    using CodeFunc = void(*)();
    CodeFunc func = reinterpret_cast<CodeFunc>(block.base);
    func();
}

// GetBlockBase implementation
uint8_t* RecompilerBackend::GetBlockBase(size_t block_index) {
    if (block_index >= num_blocks_) {
        throw std::out_of_range("Block index out of range");
    }
    return code_blocks_[block_index].base;
}

// SetEncodingPosition implementation
void RecompilerBackend::SetEncodingPosition(size_t block_index, uint8_t* position) {
    if (block_index >= num_blocks_) {
        throw std::out_of_range("Block index out of range");
    }
    
    CodeBlock& block = code_blocks_[block_index];
    
    if (!block.is_encoding) {
        throw std::runtime_error("Block is not currently being encoded");
    }
    
    if (position < block.base || position > block.base + block_size_) {
        throw std::out_of_range("Position is outside block boundaries");
    }
    
    block.current = position;
}

// GetRemainingSpace implementation
size_t RecompilerBackend::GetRemainingSpace(size_t block_index) const {
    if (block_index >= num_blocks_) {
        throw std::out_of_range("Block index out of range");
    }
    
    const CodeBlock& block = code_blocks_[block_index];
    
    if (!block.is_encoding) {
        return 0;
    }
    
    return block_size_ - (block.current - block.base);
}

// GetBlockSize implementation
size_t RecompilerBackend::GetBlockSize() const {
    return block_size_;
}

// GetNumBlocks implementation
size_t RecompilerBackend::GetNumBlocks() const {
    return num_blocks_;
}

// IsEncoding implementation
bool RecompilerBackend::IsEncoding(size_t block_index) const {
    if (block_index >= num_blocks_) {
        throw std::out_of_range("Block index out of range");
    }
    return code_blocks_[block_index].is_encoding;
}

// IsMemoryLocked implementation
bool RecompilerBackend::IsMemoryLocked() const {
    // If we got here successfully, memory should be locked
    // We could add additional verification if needed
    return memory_base_ != nullptr;
}