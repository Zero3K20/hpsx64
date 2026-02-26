/**
 * @file RecompilerBackend.h
 * @brief Backend class for a 64-bit Windows recompiler with executable memory management
 * @author 
 * @date 
 */

#pragma once

#include <Windows.h>
#include <vector>
#include <stdexcept>
#include <cstdint>

/**
 * @class RecompilerBackend
 * @brief Manages executable memory blocks for dynamic code generation and execution
 * 
 * This class provides a backend for a 64-bit Windows recompiler that manages
 * multiple code blocks of executable memory. It handles memory allocation,
 * code generation state management, and execution of generated code.
 * 
 * @note This class is not thread-safe. External synchronization is required
 *       for multi-threaded access.
 */
class RecompilerBackend {
private:
    /**
     * @struct CodeBlock
     * @brief Internal structure representing a single code block
     */
    struct CodeBlock {
        uint8_t* base;          ///< Base address of the code block
        uint8_t* current;       ///< Current encoding position within the block
        size_t size;            ///< Size of the code block in bytes
        bool is_encoding;       ///< Flag indicating if block is currently being encoded
        
        /**
         * @brief Default constructor initializes all members to safe defaults
         */
        CodeBlock();
    };
    
    std::vector<CodeBlock> code_blocks_;  ///< Container for all code blocks
    size_t block_size_;                    ///< Size of each code block in bytes
    size_t num_blocks_;                    ///< Total number of code blocks
    uint8_t* memory_base_;                 ///< Base address of all allocated memory
    
    static constexpr size_t PAGE_SIZE = 4096;  ///< Windows page size (4KB)
    
public:

    /**
     * @brief Constructs a RecompilerBackend with specified block configuration
     * 
     * Allocates a contiguous region of executable memory divided into fixed-size blocks.
     * Each block can be independently encoded and executed.
     * 
     * @param block_size_power Power of 2 for block size (minimum 12 for 4KB)
     * @param num_blocks Number of code blocks to allocate
     * 
     * @throws std::invalid_argument If block_size_power < 12 or num_blocks == 0
     * @throws std::runtime_error If memory allocation fails
     * 
     * @code
     * // Create backend with 64KB blocks (2^16 bytes), 10 blocks
     * RecompilerBackend backend(16, 10);
     * @endcode
     */
    RecompilerBackend(size_t block_size_power, size_t num_blocks);
    
    /**
     * @brief Destructor releases all allocated executable memory
     * 
     * Unlocks the memory pages and releases all allocated executable memory.
     * Any ongoing encoding operations are implicitly terminated.
     */
    ~RecompilerBackend();
    
    /**
     * @brief Deleted copy constructor to prevent copying
     */
    RecompilerBackend(const RecompilerBackend&) = delete;
    
    /**
     * @brief Deleted copy assignment operator to prevent copying
     */
    RecompilerBackend& operator=(const RecompilerBackend&) = delete;
    
    /**
     * @brief Move constructor transfers ownership of resources
     * @param other RecompilerBackend instance to move from
     */
    RecompilerBackend(RecompilerBackend&& other) noexcept;
    
    /**
     * @brief Move assignment operator transfers ownership of resources
     * @param other RecompilerBackend instance to move from
     * @return Reference to this object
     */
    RecompilerBackend& operator=(RecompilerBackend&& other) noexcept;
    
    /**
     * @brief Begins encoding in a specified code block
     * 
     * Prepares a code block for writing machine code. The block is cleared
     * with INT3 instructions (0xCC) for safety. Only one encoding session
     * can be active per block at a time.
     * 
     * @param block_index Index of the code block (0-based)
     * @return Pointer to the start of the code block for writing machine code
     * 
     * @throws std::out_of_range If block_index >= num_blocks
     * @throws std::runtime_error If the block is already being encoded
     * 
     * @code
     * uint8_t* code_ptr = backend.StartEncoding(0);
     * // Write machine code to code_ptr...
     * @endcode
     */
    uint8_t* StartEncoding(size_t block_index);
    
    /**
     * @brief Finishes encoding in a specified code block
     * 
     * Marks the end of code generation for a block and flushes the CPU
     * instruction cache to ensure the generated code is executable.
     * 
     * @param block_index Index of the code block (0-based)
     * @return Pointer to the position after the last encoded instruction
     * 
     * @throws std::out_of_range If block_index >= num_blocks
     * @throws std::runtime_error If the block is not currently being encoded
     * 
     * @note The instruction cache is automatically flushed for the modified region
     */
    uint8_t* EndEncoding(size_t block_index);
    
    /**
     * @brief Executes the code in a specified block
     * 
     * Transfers control to the generated machine code in the specified block.
     * The code is executed as a function with signature void(*)(void).
     * 
     * @param block_index Index of the code block to execute (0-based)
     * 
     * @throws std::out_of_range If block_index >= num_blocks
     * @throws std::runtime_error If the block is currently being encoded
     * 
     * @warning The generated code must follow the x64 calling convention
     *          and properly preserve non-volatile registers
     * 
     * @code
     * backend.Execute(0);  // Execute code in block 0
     * @endcode
     */
    void Execute(size_t block_index);
    
    /**
     * @brief Gets the base memory address of a code block
     * 
     * Provides direct access to the block's memory for external assemblers
     * or manual code generation.
     * 
     * @param block_index Index of the code block (0-based)
     * @return Pointer to the base address of the code block
     * 
     * @throws std::out_of_range If block_index >= num_blocks
     */
    uint8_t* GetBlockBase(size_t block_index);
    
    /**
     * @brief Updates the current encoding position within a block
     * 
     * Used when an external assembler or code generator writes directly
     * to the block memory. Allows the backend to track how much of the
     * block has been used.
     * 
     * @param block_index Index of the code block (0-based)
     * @param position New position within the block
     * 
     * @throws std::out_of_range If block_index >= num_blocks or position is outside block
     * @throws std::runtime_error If the block is not currently being encoded
     * 
     * @code
     * // After using an external assembler
     * backend.SetEncodingPosition(0, code_ptr + bytes_written);
     * @endcode
     */
    void SetEncodingPosition(size_t block_index, uint8_t* position);
    
    /**
     * @brief Gets the remaining available space in a block during encoding
     * 
     * @param block_index Index of the code block (0-based)
     * @return Number of bytes available for code generation, or 0 if not encoding
     * 
     * @throws std::out_of_range If block_index >= num_blocks
     */
    size_t GetRemainingSpace(size_t block_index) const;
    
    /**
     * @brief Gets the size of each code block
     * @return Size of each code block in bytes
     */
    size_t GetBlockSize() const;
    
    /**
     * @brief Gets the total number of code blocks
     * @return Number of allocated code blocks
     */
    size_t GetNumBlocks() const;
    
    /**
     * @brief Checks if a block is currently being encoded
     * 
     * @param block_index Index of the code block (0-based)
     * @return true if the block is in encoding state, false otherwise
     * 
     * @throws std::out_of_range If block_index >= num_blocks
     */
    bool IsEncoding(size_t block_index) const;
    
    /**
     * @brief Checks if the allocated memory is locked in physical RAM
     * 
     * @return true if memory is locked and won't be paged to disk
     * @note This is always true for properly constructed instances
     */
    bool IsMemoryLocked() const;
};