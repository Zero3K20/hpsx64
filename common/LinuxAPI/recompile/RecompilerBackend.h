/**
 * @file RecompilerBackend.h
 * @brief Backend class for a 64-bit Linux recompiler with executable memory management
 *        (Linux/POSIX port of the Windows version using mmap instead of VirtualAlloc)
 */

#pragma once

#include <vector>
#include <stdexcept>
#include <cstdint>

/**
 * @class RecompilerBackend
 * @brief Manages executable memory blocks for dynamic code generation and execution
 */
class RecompilerBackend {
private:
    struct CodeBlock {
        uint8_t* base;
        uint8_t* current;
        size_t size;
        bool is_encoding;
        CodeBlock();
    };

    std::vector<CodeBlock> code_blocks_;
    size_t block_size_;
    size_t num_blocks_;
    uint8_t* memory_base_;

    static constexpr size_t PAGE_SIZE = 4096;

public:
    RecompilerBackend(size_t block_size_power, size_t num_blocks);
    ~RecompilerBackend();

    RecompilerBackend(const RecompilerBackend&) = delete;
    RecompilerBackend& operator=(const RecompilerBackend&) = delete;

    RecompilerBackend(RecompilerBackend&& other) noexcept;
    RecompilerBackend& operator=(RecompilerBackend&& other) noexcept;

    uint8_t* StartEncoding(size_t block_index);
    uint8_t* EndEncoding(size_t block_index);
    void Execute(size_t block_index);
    uint8_t* GetBlockBase(size_t block_index);
    void SetEncodingPosition(size_t block_index, uint8_t* position);
    size_t GetRemainingSpace(size_t block_index) const;
    size_t GetBlockSize() const;
    size_t GetNumBlocks() const;
    bool IsEncoding(size_t block_index) const;
    bool IsMemoryLocked() const;
};
