/**
 * @file main.cpp
 * @brief Example usage of the RecompilerBackend class
 * @author 
 * @date 
 */

#include "RecompilerBackend.h"
#include <iostream>
#include <iomanip>

/**
 * @brief Example function that generates x64 machine code to return a value
 * @param code_ptr Pointer to memory where code will be written
 * @param value Value to return (32-bit)
 * @return Number of bytes written
 */
size_t GenerateReturnValueCode(uint8_t* code_ptr, uint32_t value) {
    size_t bytes_written = 0;
    
    // mov eax, value (5 bytes)
    code_ptr[0] = 0xB8;
    code_ptr[1] = static_cast<uint8_t>(value & 0xFF);
    code_ptr[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    code_ptr[3] = static_cast<uint8_t>((value >> 16) & 0xFF);
    code_ptr[4] = static_cast<uint8_t>((value >> 24) & 0xFF);
    bytes_written += 5;
    
    // ret (1 byte)
    code_ptr[5] = 0xC3;
    bytes_written += 1;
    
    return bytes_written;
}

/**
 * @brief Example function that generates x64 machine code for simple addition
 * @param code_ptr Pointer to memory where code will be written
 * @param a First operand (32-bit)
 * @param b Second operand (32-bit)
 * @return Number of bytes written
 */
size_t GenerateAdditionCode(uint8_t* code_ptr, uint32_t a, uint32_t b) {
    size_t bytes_written = 0;
    
    // mov eax, a (5 bytes)
    code_ptr[0] = 0xB8;
    code_ptr[1] = static_cast<uint8_t>(a & 0xFF);
    code_ptr[2] = static_cast<uint8_t>((a >> 8) & 0xFF);
    code_ptr[3] = static_cast<uint8_t>((a >> 16) & 0xFF);
    code_ptr[4] = static_cast<uint8_t>((a >> 24) & 0xFF);
    bytes_written += 5;
    
    // add eax, b (5 bytes)
    code_ptr[5] = 0x05;
    code_ptr[6] = static_cast<uint8_t>(b & 0xFF);
    code_ptr[7] = static_cast<uint8_t>((b >> 8) & 0xFF);
    code_ptr[8] = static_cast<uint8_t>((b >> 16) & 0xFF);
    code_ptr[9] = static_cast<uint8_t>((b >> 24) & 0xFF);
    bytes_written += 5;
    
    // ret (1 byte)
    code_ptr[10] = 0xC3;
    bytes_written += 1;
    
    return bytes_written;
}

/**
 * @brief Example function that generates a simple loop that prints numbers
 * @param code_ptr Pointer to memory where code will be written
 * @return Number of bytes written
 * @note This is a more complex example showing loop generation
 */
size_t GenerateSimpleLoopCode(uint8_t* code_ptr) {
    size_t bytes_written = 0;
    
    // This example generates code that would be equivalent to:
    // for (int i = 0; i < 5; i++) { /* do nothing */ }
    // return 42;
    
    // xor eax, eax (clear eax to 0) (2 bytes)
    code_ptr[0] = 0x31;
    code_ptr[1] = 0xC0;
    bytes_written += 2;
    
    // Loop start: cmp eax, 5 (3 bytes)
    code_ptr[2] = 0x83;
    code_ptr[3] = 0xF8;
    code_ptr[4] = 0x05;
    bytes_written += 3;
    
    // jge end (2 bytes) - jump if greater or equal
    code_ptr[5] = 0x7D;
    code_ptr[6] = 0x05; // Jump 5 bytes forward to skip the increment and jump back
    bytes_written += 2;
    
    // inc eax (1 byte)
    code_ptr[7] = 0x40;
    bytes_written += 1;
    
    // jmp loop_start (2 bytes) - jump back to comparison
    code_ptr[8] = 0xEB;
    code_ptr[9] = 0xF7; // Jump back 9 bytes (0x100 - 9 = 0xF7 in two's complement)
    bytes_written += 2;
    
    // End: mov eax, 42 (5 bytes)
    code_ptr[10] = 0xB8;
    code_ptr[11] = 0x2A;
    code_ptr[12] = 0x00;
    code_ptr[13] = 0x00;
    code_ptr[14] = 0x00;
    bytes_written += 5;
    
    // ret (1 byte)
    code_ptr[15] = 0xC3;
    bytes_written += 1;
    
    return bytes_written;
}

/**
 * @brief Demonstrates basic usage of RecompilerBackend
 */
void BasicUsageExample() {
    std::cout << "\n=== Basic Usage Example ===\n";
    
    try {
        // Create backend with 4KB blocks (2^12 bytes), 5 blocks
        RecompilerBackend backend(12, 5);
        
        std::cout << "Created RecompilerBackend with:\n";
        std::cout << "  Block size: " << backend.GetBlockSize() << " bytes\n";
        std::cout << "  Number of blocks: " << backend.GetNumBlocks() << "\n";
        std::cout << "  Memory locked in RAM: " << (backend.IsMemoryLocked() ? "Yes" : "No") << "\n";
        
        // Generate code that returns 42
        std::cout << "\nGenerating code to return 42...\n";
        uint8_t* code_ptr = backend.StartEncoding(0);
        size_t bytes_written = GenerateReturnValueCode(code_ptr, 42);
        backend.SetEncodingPosition(0, code_ptr + bytes_written);
        
        std::cout << "Generated " << bytes_written << " bytes of machine code\n";
        std::cout << "Remaining space in block: " << backend.GetRemainingSpace(0) << " bytes\n";
        
        // Finish encoding
        uint8_t* end_ptr = backend.EndEncoding(0);
        std::cout << "Encoding finished. Code occupies " << (end_ptr - code_ptr) << " bytes\n";
        
        // Execute the code
        std::cout << "Executing generated code...\n";
        using ReturnIntFunc = int(*)();
        ReturnIntFunc func = reinterpret_cast<ReturnIntFunc>(backend.GetBlockBase(0));
        int result = func();
        std::cout << "Function returned: " << result << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error in basic usage example: " << e.what() << "\n";
    }
}

/**
 * @brief Demonstrates multiple code blocks and more complex code generation
 */
void AdvancedUsageExample() {
    std::cout << "\n=== Advanced Usage Example ===\n";
    
    try {
        // Create backend with larger blocks
        RecompilerBackend backend(16, 3); // 64KB blocks, 3 blocks
        
        // Block 0: Addition function (15 + 27 = 42)
        std::cout << "\nBlock 0: Generating addition code (15 + 27)...\n";
        uint8_t* code_ptr = backend.StartEncoding(0);
        size_t bytes_written = GenerateAdditionCode(code_ptr, 15, 27);
        backend.SetEncodingPosition(0, code_ptr + bytes_written);
        backend.EndEncoding(0);
        
        // Block 1: Simple loop
        std::cout << "\nBlock 1: Generating loop code...\n";
        code_ptr = backend.StartEncoding(1);
        bytes_written = GenerateSimpleLoopCode(code_ptr);
        backend.SetEncodingPosition(1, code_ptr + bytes_written);
        backend.EndEncoding(1);
        
        // Block 2: Different return value
        std::cout << "\nBlock 2: Generating code to return 100...\n";
        code_ptr = backend.StartEncoding(2);
        bytes_written = GenerateReturnValueCode(code_ptr, 100);
        backend.SetEncodingPosition(2, code_ptr + bytes_written);
        backend.EndEncoding(2);
        
        // Execute all blocks and show results
        std::cout << "\nExecuting all generated code blocks:\n";
        
        using ReturnIntFunc = int(*)();
        
        for (size_t i = 0; i < 3; ++i) {
            ReturnIntFunc func = reinterpret_cast<ReturnIntFunc>(backend.GetBlockBase(i));
            int result = func();
            std::cout << "Block " << i << " returned: " << result << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error in advanced usage example: " << e.what() << "\n";
    }
}

/**
 * @brief Demonstrates error handling and edge cases
 */
void ErrorHandlingExample() {
    std::cout << "\n=== Error Handling Example ===\n";
    
    try {
        RecompilerBackend backend(12, 2); // Small blocks for testing
        
        // Test 1: Invalid block index
        std::cout << "\nTesting invalid block index...\n";
        try {
            backend.StartEncoding(10); // This should fail
        } catch (const std::out_of_range& e) {
            std::cout << "Caught expected error: " << e.what() << "\n";
        }
        
        // Test 2: Double encoding
        std::cout << "\nTesting double encoding on same block...\n";
        backend.StartEncoding(0);
        try {
            backend.StartEncoding(0); // This should fail
        } catch (const std::runtime_error& e) {
            std::cout << "Caught expected error: " << e.what() << "\n";
        }
        backend.EndEncoding(0); // Clean up
        
        // Test 3: Execute while encoding
        std::cout << "\nTesting execution while encoding...\n";
        backend.StartEncoding(1);
        try {
            backend.Execute(1); // This should fail
        } catch (const std::runtime_error& e) {
            std::cout << "Caught expected error: " << e.what() << "\n";
        }
        backend.EndEncoding(1); // Clean up
        
        std::cout << "Error handling tests completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error in error handling example: " << e.what() << "\n";
    }
}

/**
 * @brief Demonstrates checking encoding status and block information
 */
void StatusCheckingExample() {
    std::cout << "\n=== Status Checking Example ===\n";
    
    try {
        RecompilerBackend backend(13, 4); // 8KB blocks, 4 blocks
        
        // Check initial status
        std::cout << "\nInitial status of all blocks:\n";
        for (size_t i = 0; i < backend.GetNumBlocks(); ++i) {
            std::cout << "Block " << i << ": " 
                      << (backend.IsEncoding(i) ? "Encoding" : "Ready") << "\n";
        }
        
        // Start encoding on block 1
        std::cout << "\nStarting encoding on block 1...\n";
        backend.StartEncoding(1);
        
        std::cout << "Status after starting encoding:\n";
        for (size_t i = 0; i < backend.GetNumBlocks(); ++i) {
            std::cout << "Block " << i << ": " 
                      << (backend.IsEncoding(i) ? "Encoding" : "Ready");
            if (backend.IsEncoding(i)) {
                std::cout << " (Remaining space: " << backend.GetRemainingSpace(i) << " bytes)";
            }
            std::cout << "\n";
        }
        
        // Generate some code
        uint8_t* code_ptr = backend.GetBlockBase(1);
        size_t bytes_written = GenerateReturnValueCode(code_ptr, 123);
        backend.SetEncodingPosition(1, code_ptr + bytes_written);
        
        std::cout << "\nAfter writing " << bytes_written << " bytes:\n";
        std::cout << "Block 1 remaining space: " << backend.GetRemainingSpace(1) << " bytes\n";
        
        // Finish encoding
        backend.EndEncoding(1);
        std::cout << "\nAfter ending encoding:\n";
        std::cout << "Block 1 status: " << (backend.IsEncoding(1) ? "Encoding" : "Ready") << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error in status checking example: " << e.what() << "\n";
    }
}

/**
 * @brief Prints a hex dump of generated machine code
 * @param data Pointer to the data
 * @param size Number of bytes to dump
 * @param bytes_per_line Number of bytes to show per line
 */
void HexDump(const uint8_t* data, size_t size, size_t bytes_per_line = 16) {
    for (size_t i = 0; i < size; i += bytes_per_line) {
        std::cout << std::hex << std::setfill('0') << std::setw(8) << i << ": ";
        
        // Print hex bytes
        for (size_t j = 0; j < bytes_per_line && i + j < size; ++j) {
            std::cout << std::setw(2) << static_cast<int>(data[i + j]) << " ";
        }
        
        // Pad if necessary
        for (size_t j = size - i; j < bytes_per_line && j < size; ++j) {
            std::cout << "   ";
        }
        
        std::cout << " | ";
        
        // Print ASCII representation
        for (size_t j = 0; j < bytes_per_line && i + j < size; ++j) {
            uint8_t byte = data[i + j];
            std::cout << (isprint(byte) ? static_cast<char>(byte) : '.');
        }
        
        std::cout << std::dec << "\n";
    }
}

/**
 * @brief Demonstrates inspection of generated machine code
 */
void CodeInspectionExample() {
    std::cout << "\n=== Code Inspection Example ===\n";
    
    try {
        RecompilerBackend backend(12, 1);
        
        // Generate code and inspect it
        uint8_t* code_ptr = backend.StartEncoding(0);
        size_t bytes_written = GenerateAdditionCode(code_ptr, 0x12345678, 0x87654321);
        backend.SetEncodingPosition(0, code_ptr + bytes_written);
        backend.EndEncoding(0);
        
        std::cout << "\nGenerated machine code for addition (0x12345678 + 0x87654321):\n";
        std::cout << "Size: " << bytes_written << " bytes\n\n";
        
        HexDump(code_ptr, bytes_written);
        
        // Execute and show result
        using ReturnIntFunc = uint32_t(*)();
        ReturnIntFunc func = reinterpret_cast<ReturnIntFunc>(code_ptr);
        uint32_t result = func();
        std::cout << "\nExecution result: 0x" << std::hex << result << std::dec 
                  << " (" << result << ")\n";
        std::cout << "Expected: 0x" << std::hex << (0x12345678U + 0x87654321U) << std::dec 
                  << " (" << (0x12345678U + 0x87654321U) << ")\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error in code inspection example: " << e.what() << "\n";
    }
}

/**
 * @brief Main function demonstrating various uses of RecompilerBackend
 */
int main() {
    std::cout << "RecompilerBackend Example Application\n";
    std::cout << "=====================================\n";
    
    // Run all examples
    BasicUsageExample();
    AdvancedUsageExample();
    ErrorHandlingExample();
    StatusCheckingExample();
    CodeInspectionExample();
    
    std::cout << "\n=== All Examples Completed ===\n";
    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    
    return 0;
}