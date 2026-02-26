// main.cpp
// Test program demonstrating 8/16/32/64-bit operations

#include "x64_assembler.h"
#include <cstdio>

using namespace x64asm;

int main() {
    printf("x64 Assembler - Multi-size Operations Test\n");
    printf("===========================================\n\n");
    
    UniversalRipAssembler asm_obj(0x140001000);

    // ========================================================================
    // Example 1: 64-bit Operations
    // ========================================================================
    printf("=== Example 1: 64-bit Operations ===\n");
    
    asm_obj.reset();
    printf("add rcx, 1\nret\n");
    asm_obj.emit("add rcx, 1");
    asm_obj.emit("ret");
    asm_obj.print_code();
    printf("\nREX.W prefix (48) for 64-bit\n\n");

    asm_obj.reset();
    printf("add rax, rcx\n");
    asm_obj.emit("add rax, rcx");
    asm_obj.print_code();

    asm_obj.reset();
    printf("mov rax, 0x1234567890ABCDEF\n");
    asm_obj.emit("mov rax, 0x1234567890ABCDEF");
    asm_obj.print_code();
    printf("\n64-bit immediate\n\n");

    asm_obj.reset();
    printf("ret\n");
    asm_obj.emit("ret");
    asm_obj.print_code();

    asm_obj.reset();
    printf("inc eax\n");
    asm_obj.emit("inc eax");
    asm_obj.print_code();

    asm_obj.reset();
    printf("mov ecx, 3\n");
    asm_obj.emit("mov ecx, 3");
    asm_obj.print_code();

    asm_obj.reset();
    printf("mov rcx, 5\n");
    asm_obj.emit("mov rcx, 5");
    asm_obj.print_code();


    // ========================================================================
    // Example 2: 32-bit Operations
    // ========================================================================
    printf("=== Example 2: 32-bit Operations ===\n");
    
    asm_obj.reset();
    printf("add eax, ecx\n");
    asm_obj.emit("add eax, ecx");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - No REX prefix for 32-bit\n\n", size);
    }

    asm_obj.reset();
    printf("add eax, 42\n");
    asm_obj.emit("add eax, 42");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - 8-bit immediate (sign-extended)\n\n", size);
    }

    asm_obj.reset();
    printf("mov ebx, 0x12345678\n");
    asm_obj.emit("mov ebx, 0x12345678");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - 32-bit immediate\n\n", size);
    }

    asm_obj.reset();
    printf("xor edx, edx\n");
    asm_obj.emit("xor edx, edx");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - Common zero-out pattern\n\n", size);
    }

    // ========================================================================
    // Example 3: 16-bit Operations
    // ========================================================================
    printf("=== Example 3: 16-bit Operations ===\n");
    
    asm_obj.reset();
    printf("add ax, cx\n");
    asm_obj.emit("add ax, cx");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - 0x66 prefix for 16-bit\n\n", size);
    }

    asm_obj.reset();
    printf("mov bx, 0x1234\n");
    asm_obj.emit("mov bx, 0x1234");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - 16-bit immediate\n\n", size);
    }

    asm_obj.reset();
    printf("sub dx, 100\n");
    asm_obj.emit("sub dx, 100");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes)\n\n", size);
    }

    // ========================================================================
    // Example 4: 8-bit Operations
    // ========================================================================
    printf("=== Example 4: 8-bit Operations ===\n");
    
    asm_obj.reset();
    printf("add al, cl\n");
    asm_obj.emit("add al, cl");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - Byte operation\n\n", size);
    }

    asm_obj.reset();
    printf("mov bl, 255\n");
    asm_obj.emit("mov bl, 255");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - 8-bit immediate\n\n", size);
    }

    asm_obj.reset();
    printf("xor dh, dh\n");
    asm_obj.emit("xor dh, dh");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - High byte register\n\n", size);
    }

    // ========================================================================
    // Example 5: Extended Registers (R8-R15)
    // ========================================================================
    printf("=== Example 5: Extended Registers (R8-R15) ===\n");
    
    asm_obj.reset();
    printf("add r8, r9\n");
    asm_obj.emit("add r8, r9");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - REX prefix for extended regs\n\n", size);
    }

    asm_obj.reset();
    printf("add r10d, r11d\n");
    asm_obj.emit("add r10d, r11d");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - 32-bit extended regs\n\n", size);
    }

    asm_obj.reset();
    printf("mov r12w, r13w\n");
    asm_obj.emit("mov r12w, r13w");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - 16-bit extended regs with 0x66 prefix\n\n", size);
    }

    asm_obj.reset();
    printf("add r14b, r15b\n");
    asm_obj.emit("add r14b, r15b");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - 8-bit extended regs\n\n", size);
    }

    // ========================================================================
    // Example 6: Special Low Byte Registers (SPL, BPL, SIL, DIL)
    // ========================================================================
    printf("=== Example 6: Special Low Byte Registers ===\n");
    
    asm_obj.reset();
    printf("mov sil, 42\n");
    asm_obj.emit("mov sil, 42");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - Requires REX prefix\n\n", size);
    }

    asm_obj.reset();
    printf("add dil, bpl\n");
    asm_obj.emit("add dil, bpl");
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes) - REX required for these low bytes\n\n", size);
    }

    // ========================================================================
    // Example 7: Mixed Size Operations
    // ========================================================================
    printf("=== Example 7: Mixed Size Demonstration ===\n");
    
    asm_obj.reset();
    printf("Building a function with mixed sizes:\n");
    printf("  mov eax, 100       ; 32-bit\n");
    asm_obj.emit("mov eax, 100");
    printf("  add ax, 10         ; 16-bit\n");
    asm_obj.emit("add ax, 10");
    printf("  sub al, 5          ; 8-bit\n");
    asm_obj.emit("sub al, 5");
    printf("  mov rbx, rax       ; 64-bit\n");
    asm_obj.emit("mov rbx, rax");
    
    {
        printf("\nFull code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes total)\n\n", size);
    }

    // ========================================================================
    // Example 8: Comparison with Placeholders
    // ========================================================================
    printf("=== Example 8: Placeholders with Different Sizes ===\n");
    
    asm_obj.reset();
    printf("add @reg0, @imm0 (with 32-bit register)\n");
    asm_obj.emit("add @reg0, @imm0", Reg::EAX, 500);
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes)\n\n", size);
    }

    asm_obj.reset();
    printf("add @reg0, @imm0 (with 16-bit register)\n");
    asm_obj.emit("add @reg0, @imm0", Reg::AX, 500);
    {
        printf("Code: ");
        const uint8_t* code = asm_obj.get_code();
        size_t size = asm_obj.get_size();
        for (size_t i = 0; i < size; ++i) printf("%02X ", code[i]);
        printf("\n(%zu bytes)\n\n", size);
    }

    // ========================================================================
    // Summary
    // ========================================================================
    printf("========================================\n");
    printf("Multi-size Encoding Summary\n");
    printf("========================================\n\n");
    
    printf("64-bit operations (QWORD):\n");
    printf("  - Use 'r' prefix registers (rax, rcx, r8, etc.)\n");
    printf("  - REX.W prefix (0x48) required\n");
    printf("  - Example: add rax, rcx -> 48 01 C8\n\n");
    
    printf("32-bit operations (DWORD):\n");
    printf("  - Use 'e' prefix registers (eax, ecx, etc.) or r#d (r8d, r9d)\n");
    printf("  - No size prefix needed\n");
    printf("  - Example: add eax, ecx -> 01 C8\n\n");
    
    printf("16-bit operations (WORD):\n");
    printf("  - Use 2-letter registers (ax, cx, etc.) or r#w (r8w, r9w)\n");
    printf("  - 0x66 operand size override prefix required\n");
    printf("  - Example: add ax, cx -> 66 01 C8\n\n");
    
    printf("8-bit operations (BYTE):\n");
    printf("  - Use single-letter low bytes (al, cl, etc.)\n");
    printf("  - Use high bytes (ah, bh, ch, dh)\n");
    printf("  - Use low bytes with REX (spl, bpl, sil, dil)\n");
    printf("  - Use r#b for extended regs (r8b-r15b)\n");
    printf("  - Byte opcode variant used\n");
    printf("  - Example: add al, cl -> 00 C8\n\n");
    
    printf("Extended Registers (R8-R15):\n");
    printf("  - Require REX prefix to access\n");
    printf("  - REX.R bit set for register in ModR/M reg field\n");
    printf("  - REX.B bit set for register in ModR/M r/m field\n\n");
    
    printf("Compilation: cl /std:c++17 /EHsc /O2 x64_assembler.cpp main.cpp\n");
    
    return 0;
}