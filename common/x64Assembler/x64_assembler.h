// x64_assembler.h
/**
 * @file x64_assembler.h
 * @brief Universal x64 Assembler with Multi-size Operations (8/16/32/64-bit)
 * @author MSVC C++17 compatible for Windows x64
 */

#ifndef X64_ASSEMBLER_H
#define X64_ASSEMBLER_H

#include <array>
#include <string_view>
#include <cstdint>
#include <vector>
#include <type_traits>

namespace x64asm {

// VEX/EVEX prefix encoding (pp field)
constexpr uint8_t VEX_PREFIX_NONE = 0x00;  // No prefix
constexpr uint8_t VEX_PREFIX_66 = 0x01;  // 0x66 operand-size override
constexpr uint8_t VEX_PREFIX_F3 = 0x02;  // 0xF3 REPZ
constexpr uint8_t VEX_PREFIX_F2 = 0x03;  // 0xF2 REPNZ

enum class RegSize : uint8_t {
    QWORD = 64, DWORD = 32, WORD = 16, BYTE = 8
};

enum class Reg : uint8_t {
    // 64-bit registers
    RAX = 0, RCX = 1, RDX = 2, RBX = 3, RSP = 4, RBP = 5, RSI = 6, RDI = 7,
    R8 = 8, R9 = 9, R10 = 10, R11 = 11, R12 = 12, R13 = 13, R14 = 14, R15 = 15,
    // 32-bit registers
    EAX = 0, ECX = 1, EDX = 2, EBX = 3, ESP = 4, EBP = 5, ESI = 6, EDI = 7,
    R8D = 8, R9D = 9, R10D = 10, R11D = 11, R12D = 12, R13D = 13, R14D = 14, R15D = 15,
    // 16-bit registers
    AX = 0, CX = 1, DX = 2, BX = 3, SP = 4, BP = 5, SI = 6, DI = 7,
    R8W = 8, R9W = 9, R10W = 10, R11W = 11, R12W = 12, R13W = 13, R14W = 14, R15W = 15,
    // 8-bit registers
    AL = 0, CL = 1, DL = 2, BL = 3, AH = 4, CH = 5, DH = 6, BH = 7,
    SPL = 4, BPL = 5, SIL = 6, DIL = 7,
    R8B = 8, R9B = 9, R10B = 10, R11B = 11, R12B = 12, R13B = 13, R14B = 14, R15B = 15,
    // Vector registers
    XMM0 = 0, XMM1 = 1, XMM2 = 2, XMM3 = 3, XMM4 = 4, XMM5 = 5, XMM6 = 6, XMM7 = 7,
    XMM8 = 8, XMM9 = 9, XMM10 = 10, XMM11 = 11, XMM12 = 12, XMM13 = 13, XMM14 = 14, XMM15 = 15,
    XMM16 = 16, XMM17 = 17, XMM18 = 18, XMM19 = 19, XMM20 = 20, XMM21 = 21, XMM22 = 22, XMM23 = 23,
    XMM24 = 24, XMM25 = 25, XMM26 = 26, XMM27 = 27, XMM28 = 28, XMM29 = 29, XMM30 = 30, XMM31 = 31,
    // Mask registers
    K0 = 0, K1 = 1, K2 = 2, K3 = 3, K4 = 4, K5 = 5, K6 = 6, K7 = 7,
    NONE = 255
};

enum class MaskMode : uint8_t {
    None = 0, Merge = 1, Zero = 2
};

enum class OperandType {
    Register, Immediate, Memory, PointerPlaceholder, ImmediatePlaceholder, 
    JumpLabel, LongJumpLabel, RegisterPlaceholder
};

struct MemoryOperand {
    Reg base, index;
    uint8_t scale;
    int32_t displacement;
    bool has_base, has_index, has_displacement;
    constexpr MemoryOperand() : base(Reg::RAX), index(Reg::RAX), scale(1), 
        displacement(0), has_base(false), has_index(false), has_displacement(false) {}
};

struct Operand {
    OperandType type;
    RegSize reg_size;
    union {
        Reg reg;
        uint64_t immediate;
        MemoryOperand memory;
        uint8_t placeholder_id;
    };
    Reg opmask;
    MaskMode mask_mode;
    constexpr Operand() : type(OperandType::Register), reg_size(RegSize::QWORD), reg(Reg::RAX), 
        opmask(Reg::NONE), mask_mode(MaskMode::None) {}
    constexpr Operand(Reg r, RegSize size) : type(OperandType::Register), reg_size(size), reg(r), 
        opmask(Reg::NONE), mask_mode(MaskMode::None) {}
    constexpr Operand(uint64_t imm) : type(OperandType::Immediate), reg_size(RegSize::QWORD), immediate(imm), 
        opmask(Reg::NONE), mask_mode(MaskMode::None) {}
    constexpr Operand(const MemoryOperand& mem) : type(OperandType::Memory), reg_size(RegSize::QWORD), memory(mem), 
        opmask(Reg::NONE), mask_mode(MaskMode::None) {}
    constexpr Operand(OperandType t, uint8_t pid) : type(t), reg_size(RegSize::QWORD), placeholder_id(pid), 
        opmask(Reg::NONE), mask_mode(MaskMode::None) {}
};

enum class EncodingType {
    REG_REG, REG_IMM, REG_IMM8, REG_MEM, MEM_REG, MEM_IMM, MEM_IMM8,
    SINGLE_REG, SINGLE_MEM, NO_OPERANDS, VEX_REG_REG_REG, VEX_REG_REG_MEM,
    EVEX_REG_REG_REG, EVEX_REG_REG_MEM, JUMP_LABEL, LONG_JUMP_LABEL, ACC_IMM,
    REG_CL, MEM_CL,
    REG_REG_IMM, REG_REG_IMM8, REG_MEM_IMM, REG_MEM_IMM8,
    JUMP_PTR, CALL_PTR, JUMP_ABS, CALL_ABS,
    VEX_REG_REG, EVEX_REG_REG,
    VEX_MEM_REG, EVEX_MEM_REG,
    VEX_REG_MEM, EVEX_REG_MEM,
    VEX_REG_REG_IMM8, EVEX_REG_REG_IMM8,
    VEX_REG_MEM_IMM8, EVEX_REG_MEM_IMM8,
    VEX_MEM_REG_IMM8, EVEX_MEM_REG_IMM8,

    // 4-operand forms (destination, src1, src2, imm8/mask)
    VEX_REG_REG_REG_IMM8,      // vpblendw xmm1, xmm2, xmm3, imm8
    VEX_REG_REG_MEM_IMM8,      // vpblendw xmm1, xmm2, [mem], imm8
    EVEX_REG_REG_REG_IMM8,     // vpblendw xmm1, xmm2, xmm3, imm8 (with mask)
    EVEX_REG_REG_MEM_IMM8,     // vpblendw xmm1, xmm2, [mem], imm8 (with mask)

    // 4-operand forms with register mask (vpblendvb-style)
    VEX_REG_REG_REG_REG,       // vpblendvb xmm1, xmm2, xmm3, xmm4
    VEX_REG_REG_MEM_REG,       // vpblendvb xmm1, xmm2, [mem], xmm4
};

enum class VexType : uint8_t {
    NONE = 0, VEX_128 = 1, VEX_256 = 2,
    EVEX_128 = 3, EVEX_256 = 4, EVEX_512 = 5
};

// Helper to check if 2-byte VEX encoding is possible
constexpr bool can_use_2byte_vex(uint8_t vex_map, uint8_t rex_r, uint8_t rex_x, uint8_t rex_b, bool rex_w) {
    // 2-byte VEX requirements:
    // - Must be map 1 (0x0F opcodes)
    // - No REX.X, REX.B, or REX.W bits
    // - REX.R is allowed (encoded in VEX)
    return (vex_map == 0x01) && !rex_x && !rex_b && !rex_w;
}

struct InstructionEncoding {
    const char* mnemonic;
    EncodingType type;
    uint8_t prefix, opcode, opcode2, opcode3, reg_field;
    bool needs_rex_w, has_modrm, is_two_byte, is_three_byte, supports_rip_relative;
    VexType vex_type;
    uint8_t vex_map;
    bool vex_w;
};

template<size_t MaxSize = 512>
struct RuntimeInstructionBuffer {
    std::array<uint8_t, MaxSize> data;
    size_t size;
    uint64_t base_address;
    uint8_t* external_buffer;
    bool use_external;
    size_t max_size;
    bool size_limit_enabled;

    RuntimeInstructionBuffer(uint64_t addr = 0)
        : data{}, size(0), base_address(addr), external_buffer(nullptr),
        use_external(false), max_size(MaxSize), size_limit_enabled(false) {
    }

    void emit(uint8_t byte) {
        if (size_limit_enabled && size >= max_size) {
            printf("ERROR: Buffer overflow! Attempted to write beyond buffer limit of %zu bytes.\n", max_size);
            printf("       Current size: %zu, attempted to add 1 more byte.\n", size);
            return;  // Don't write beyond limit
        }

        if (use_external) {
            external_buffer[size++] = byte;
        }
        else {
            data[size++] = byte;
        }
    }

    void emit_dword(uint32_t v) {
        emit(uint8_t(v)); emit(uint8_t(v >> 8));
        emit(uint8_t(v >> 16)); emit(uint8_t(v >> 24));
    }

    void emit_qword(uint64_t v) {
        emit_dword(uint32_t(v)); emit_dword(uint32_t(v >> 32));
    }

    uint64_t current_address() const { return base_address + size; }

    const uint8_t* get_data() const {
        return use_external ? external_buffer : data.data();
    }
};

namespace parser {
    constexpr bool is_whitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
    constexpr bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
    constexpr bool is_digit(char c) { return c >= '0' && c <= '9'; }
    constexpr char to_lower(char c) { return (c >= 'A' && c <= 'Z') ? c + 32 : c; }
    
    constexpr std::string_view skip_whitespace(std::string_view str) {
        while (!str.empty() && is_whitespace(str.front())) str.remove_prefix(1);
        return str;
    }
    
    constexpr std::string_view skip_comma(std::string_view str) {
        str = skip_whitespace(str);
        if (!str.empty() && str.front() == ',') str.remove_prefix(1);
        return skip_whitespace(str);
    }
    
    constexpr std::pair<std::string_view, std::string_view> parse_identifier(std::string_view str) {
        str = skip_whitespace(str);
        const char* start = str.data();
        size_t len = 0;
        while (!str.empty() && (is_alpha(str.front()) || is_digit(str.front()) || str.front() == '@')) {
            str.remove_prefix(1);
            len++;
        }
        return {std::string_view{start, len}, str};
    }
    
    constexpr std::pair<uint64_t, std::string_view> parse_number(std::string_view str) {
        str = skip_whitespace(str);
        bool negative = false;
        if (!str.empty() && str.front() == '-') {
            negative = true;
            str.remove_prefix(1);
            str = skip_whitespace(str);
        }
        uint64_t result = 0;
        if (str.size() >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            str.remove_prefix(2);
            while (!str.empty()) {
                char c = str.front();
                if (c >= '0' && c <= '9') result = result * 16 + (c - '0');
                else if (c >= 'a' && c <= 'f') result = result * 16 + (c - 'a' + 10);
                else if (c >= 'A' && c <= 'F') result = result * 16 + (c - 'A' + 10);
                else break;
                str.remove_prefix(1);
            }
        } else {
            while (!str.empty() && is_digit(str.front())) {
                result = result * 10 + (str.front() - '0');
                str.remove_prefix(1);
            }
        }
        if (negative) {
            result = static_cast<uint64_t>(-static_cast<int64_t>(result));
        }
        return {result, str};
    }
    
    constexpr bool str_equal(std::string_view a, const char* b) {
        if (b == nullptr) return false;
        size_t b_len = 0;
        while (b[b_len] != '\0') ++b_len;
        if (a.size() != b_len) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (to_lower(a[i]) != to_lower(b[i])) return false;
        }
        return true;
    }
    
    constexpr std::pair<Operand, std::string_view> parse_register(std::string_view str) {
        auto name_pair = parse_identifier(str);
        std::string_view name = name_pair.first;
        std::string_view rem = name_pair.second;
        
        // 64-bit registers
        if (str_equal(name, "rax")) return {Operand{Reg::RAX, RegSize::QWORD}, rem};
        if (str_equal(name, "rcx")) return {Operand{Reg::RCX, RegSize::QWORD}, rem};
        if (str_equal(name, "rdx")) return {Operand{Reg::RDX, RegSize::QWORD}, rem};
        if (str_equal(name, "rbx")) return {Operand{Reg::RBX, RegSize::QWORD}, rem};
        if (str_equal(name, "rsp")) return {Operand{Reg::RSP, RegSize::QWORD}, rem};
        if (str_equal(name, "rbp")) return {Operand{Reg::RBP, RegSize::QWORD}, rem};
        if (str_equal(name, "rsi")) return {Operand{Reg::RSI, RegSize::QWORD}, rem};
        if (str_equal(name, "rdi")) return {Operand{Reg::RDI, RegSize::QWORD}, rem};
        if (str_equal(name, "r8")) return {Operand{Reg::R8, RegSize::QWORD}, rem};
        if (str_equal(name, "r9")) return {Operand{Reg::R9, RegSize::QWORD}, rem};
        if (str_equal(name, "r10")) return {Operand{Reg::R10, RegSize::QWORD}, rem};
        if (str_equal(name, "r11")) return {Operand{Reg::R11, RegSize::QWORD}, rem};
        if (str_equal(name, "r12")) return {Operand{Reg::R12, RegSize::QWORD}, rem};
        if (str_equal(name, "r13")) return {Operand{Reg::R13, RegSize::QWORD}, rem};
        if (str_equal(name, "r14")) return {Operand{Reg::R14, RegSize::QWORD}, rem};
        if (str_equal(name, "r15")) return {Operand{Reg::R15, RegSize::QWORD}, rem};
        
        // 32-bit registers
        if (str_equal(name, "eax")) return {Operand{Reg::EAX, RegSize::DWORD}, rem};
        if (str_equal(name, "ecx")) return {Operand{Reg::ECX, RegSize::DWORD}, rem};
        if (str_equal(name, "edx")) return {Operand{Reg::EDX, RegSize::DWORD}, rem};
        if (str_equal(name, "ebx")) return {Operand{Reg::EBX, RegSize::DWORD}, rem};
        if (str_equal(name, "esp")) return {Operand{Reg::ESP, RegSize::DWORD}, rem};
        if (str_equal(name, "ebp")) return {Operand{Reg::EBP, RegSize::DWORD}, rem};
        if (str_equal(name, "esi")) return {Operand{Reg::ESI, RegSize::DWORD}, rem};
        if (str_equal(name, "edi")) return {Operand{Reg::EDI, RegSize::DWORD}, rem};
        if (str_equal(name, "r8d")) return {Operand{Reg::R8D, RegSize::DWORD}, rem};
        if (str_equal(name, "r9d")) return {Operand{Reg::R9D, RegSize::DWORD}, rem};
        if (str_equal(name, "r10d")) return {Operand{Reg::R10D, RegSize::DWORD}, rem};
        if (str_equal(name, "r11d")) return {Operand{Reg::R11D, RegSize::DWORD}, rem};
        if (str_equal(name, "r12d")) return {Operand{Reg::R12D, RegSize::DWORD}, rem};
        if (str_equal(name, "r13d")) return {Operand{Reg::R13D, RegSize::DWORD}, rem};
        if (str_equal(name, "r14d")) return {Operand{Reg::R14D, RegSize::DWORD}, rem};
        if (str_equal(name, "r15d")) return {Operand{Reg::R15D, RegSize::DWORD}, rem};
        
        // 16-bit registers
        if (str_equal(name, "ax")) return {Operand{Reg::AX, RegSize::WORD}, rem};
        if (str_equal(name, "cx")) return {Operand{Reg::CX, RegSize::WORD}, rem};
        if (str_equal(name, "dx")) return {Operand{Reg::DX, RegSize::WORD}, rem};
        if (str_equal(name, "bx")) return {Operand{Reg::BX, RegSize::WORD}, rem};
        if (str_equal(name, "sp")) return {Operand{Reg::SP, RegSize::WORD}, rem};
        if (str_equal(name, "bp")) return {Operand{Reg::BP, RegSize::WORD}, rem};
        if (str_equal(name, "si")) return {Operand{Reg::SI, RegSize::WORD}, rem};
        if (str_equal(name, "di")) return {Operand{Reg::DI, RegSize::WORD}, rem};
        if (str_equal(name, "r8w")) return {Operand{Reg::R8W, RegSize::WORD}, rem};
        if (str_equal(name, "r9w")) return {Operand{Reg::R9W, RegSize::WORD}, rem};
        if (str_equal(name, "r10w")) return {Operand{Reg::R10W, RegSize::WORD}, rem};
        if (str_equal(name, "r11w")) return {Operand{Reg::R11W, RegSize::WORD}, rem};
        if (str_equal(name, "r12w")) return {Operand{Reg::R12W, RegSize::WORD}, rem};
        if (str_equal(name, "r13w")) return {Operand{Reg::R13W, RegSize::WORD}, rem};
        if (str_equal(name, "r14w")) return {Operand{Reg::R14W, RegSize::WORD}, rem};
        if (str_equal(name, "r15w")) return {Operand{Reg::R15W, RegSize::WORD}, rem};
        
        // 8-bit registers
        if (str_equal(name, "al")) return {Operand{Reg::AL, RegSize::BYTE}, rem};
        if (str_equal(name, "cl")) return {Operand{Reg::CL, RegSize::BYTE}, rem};
        if (str_equal(name, "dl")) return {Operand{Reg::DL, RegSize::BYTE}, rem};
        if (str_equal(name, "bl")) return {Operand{Reg::BL, RegSize::BYTE}, rem};
        if (str_equal(name, "ah")) return {Operand{Reg::AH, RegSize::BYTE}, rem};
        if (str_equal(name, "ch")) return {Operand{Reg::CH, RegSize::BYTE}, rem};
        if (str_equal(name, "dh")) return {Operand{Reg::DH, RegSize::BYTE}, rem};
        if (str_equal(name, "bh")) return {Operand{Reg::BH, RegSize::BYTE}, rem};
        if (str_equal(name, "spl")) return {Operand{Reg::SPL, RegSize::BYTE}, rem};
        if (str_equal(name, "bpl")) return {Operand{Reg::BPL, RegSize::BYTE}, rem};
        if (str_equal(name, "sil")) return {Operand{Reg::SIL, RegSize::BYTE}, rem};
        if (str_equal(name, "dil")) return {Operand{Reg::DIL, RegSize::BYTE}, rem};
        if (str_equal(name, "r8b")) return {Operand{Reg::R8B, RegSize::BYTE}, rem};
        if (str_equal(name, "r9b")) return {Operand{Reg::R9B, RegSize::BYTE}, rem};
        if (str_equal(name, "r10b")) return {Operand{Reg::R10B, RegSize::BYTE}, rem};
        if (str_equal(name, "r11b")) return {Operand{Reg::R11B, RegSize::BYTE}, rem};
        if (str_equal(name, "r12b")) return {Operand{Reg::R12B, RegSize::BYTE}, rem};
        if (str_equal(name, "r13b")) return {Operand{Reg::R13B, RegSize::BYTE}, rem};
        if (str_equal(name, "r14b")) return {Operand{Reg::R14B, RegSize::BYTE}, rem};
        if (str_equal(name, "r15b")) return {Operand{Reg::R15B, RegSize::BYTE}, rem};
        
        // Mask registers
        if (str_equal(name, "k0")) return {Operand{Reg::K0, RegSize::QWORD}, rem};
        if (str_equal(name, "k1")) return {Operand{Reg::K1, RegSize::QWORD}, rem};
        if (str_equal(name, "k2")) return {Operand{Reg::K2, RegSize::QWORD}, rem};
        if (str_equal(name, "k3")) return {Operand{Reg::K3, RegSize::QWORD}, rem};
        if (str_equal(name, "k4")) return {Operand{Reg::K4, RegSize::QWORD}, rem};
        if (str_equal(name, "k5")) return {Operand{Reg::K5, RegSize::QWORD}, rem};
        if (str_equal(name, "k6")) return {Operand{Reg::K6, RegSize::QWORD}, rem};
        if (str_equal(name, "k7")) return {Operand{Reg::K7, RegSize::QWORD}, rem};
        
        // Vector registers
        if (name.size() >= 4) {
            char c0 = to_lower(name[0]);
            char c1 = to_lower(name[1]);
            char c2 = to_lower(name[2]);
            if ((c0 == 'x' || c0 == 'y' || c0 == 'z') && c1 == 'm' && c2 == 'm') {
                std::string_view num_str = name.substr(3);
                uint64_t reg_num = 0;
                for (char c : num_str) {
                    if (c >= '0' && c <= '9') reg_num = reg_num * 10 + (c - '0');
                    else break;
                }
                if (reg_num <= 31) return {Operand{static_cast<Reg>(reg_num), RegSize::QWORD}, rem};
            }
        }
        return {Operand{Reg::RAX, RegSize::QWORD}, rem};
    }
    
    constexpr std::pair<Operand, std::string_view> parse_placeholder(std::string_view str) {
        auto id_pair = parse_identifier(str);
        std::string_view id = id_pair.first;
        std::string_view rem = id_pair.second;
        
        if (str_equal(id, "@ptr") || str_equal(id, "@ptr0")) return {Operand{OperandType::PointerPlaceholder, 0}, rem};
        if (str_equal(id, "@ptr1")) return {Operand{OperandType::PointerPlaceholder, 1}, rem};
        if (str_equal(id, "@ptr2")) return {Operand{OperandType::PointerPlaceholder, 2}, rem};
        if (str_equal(id, "@ptr3")) return {Operand{OperandType::PointerPlaceholder, 3}, rem};
        if (str_equal(id, "@imm") || str_equal(id, "@imm0")) return {Operand{OperandType::ImmediatePlaceholder, 0}, rem};
        if (str_equal(id, "@imm1")) return {Operand{OperandType::ImmediatePlaceholder, 1}, rem};
        if (str_equal(id, "@imm2")) return {Operand{OperandType::ImmediatePlaceholder, 2}, rem};
        if (str_equal(id, "@imm3")) return {Operand{OperandType::ImmediatePlaceholder, 3}, rem};
        if (str_equal(id, "@reg") || str_equal(id, "@reg0")) return {Operand{OperandType::RegisterPlaceholder, 0}, rem};
        if (str_equal(id, "@reg1")) return {Operand{OperandType::RegisterPlaceholder, 1}, rem};
        if (str_equal(id, "@reg2")) return {Operand{OperandType::RegisterPlaceholder, 2}, rem};
        if (str_equal(id, "@reg3")) return {Operand{OperandType::RegisterPlaceholder, 3}, rem};
        
        if (id.size() >= 9 && id[0] == '@' && id[1] == 'l' && id[2] == 'o' && id[3] == 'n' && 
            id[4] == 'g' && id[5] == 'j' && id[6] == 'm' && id[7] == 'p') {
            uint8_t label_id = 0;
            for (size_t i = 8; i < id.size(); ++i) {
                if (id[i] >= '0' && id[i] <= '9') {
                    label_id = label_id * 10 + (id[i] - '0');
                } else break;
            }
            if (label_id < 16) return {Operand{OperandType::LongJumpLabel, label_id}, rem};
        }
        
        if (id.size() >= 5 && id[0] == '@' && id[1] == 'j' && id[2] == 'm' && id[3] == 'p') {
            uint8_t label_id = 0;
            for (size_t i = 4; i < id.size(); ++i) {
                if (id[i] >= '0' && id[i] <= '9') {
                    label_id = label_id * 10 + (id[i] - '0');
                } else break;
            }
            if (label_id < 16) return {Operand{OperandType::JumpLabel, label_id}, rem};
        }
        
        return {Operand{}, rem};
    }
    
    constexpr std::pair<MemoryOperand, std::string_view> parse_memory(std::string_view str) {
        MemoryOperand mem{};
        str = skip_whitespace(str);
        if (str.empty() || str.front() != '[') return {mem, str};
        str.remove_prefix(1);
        str = skip_whitespace(str);
        
        if (!str.empty() && is_alpha(str.front())) {
            auto base_pair = parse_register(str);
            mem.base = base_pair.first.reg;
            mem.has_base = true;
            str = skip_whitespace(base_pair.second);
        }
        
        while (!str.empty() && (str.front() == '+' || str.front() == '-')) {
            bool neg = str.front() == '-';
            str.remove_prefix(1);
            str = skip_whitespace(str);
            
            if (!str.empty() && is_alpha(str.front())) {
                auto idx_pair = parse_register(str);
                mem.index = idx_pair.first.reg;
                mem.has_index = true;
                str = skip_whitespace(idx_pair.second);
                
                if (!str.empty() && str.front() == '*') {
                    str.remove_prefix(1);
                    auto scale_pair = parse_number(skip_whitespace(str));
                    uint64_t scale_val = scale_pair.first;
                    if (scale_val == 1 || scale_val == 2 || scale_val == 4 || scale_val == 8) 
                        mem.scale = static_cast<uint8_t>(scale_val);
                    str = skip_whitespace(scale_pair.second);
                }
            } else if (!str.empty() && (is_digit(str.front()) || 
                      (str.size() >= 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')))) {
                auto disp_pair = parse_number(str);
                mem.displacement = neg ? -static_cast<int32_t>(disp_pair.first) : static_cast<int32_t>(disp_pair.first);
                mem.has_displacement = true;
                str = skip_whitespace(disp_pair.second);
            }
        }
        
        if (!str.empty() && str.front() == ']') str.remove_prefix(1);
        return {mem, str};
    }
    
    constexpr std::pair<std::pair<Reg, MaskMode>, std::string_view> parse_mask_decorator(std::string_view str) {
        str = skip_whitespace(str);
        if (str.empty() || str.front() != '{') return {{Reg::NONE, MaskMode::None}, str};
        
        str.remove_prefix(1);
        str = skip_whitespace(str);
        auto mask_pair = parse_register(str);
        Reg mask_reg = mask_pair.first.reg;
        str = skip_whitespace(mask_pair.second);
        
        if (str.empty() || str.front() != '}') return {{Reg::NONE, MaskMode::None}, str};
        str.remove_prefix(1);
        
        MaskMode mode = MaskMode::Merge;
        str = skip_whitespace(str);
        
        if (!str.empty() && str.front() == '{') {
            str.remove_prefix(1);
            str = skip_whitespace(str);
            if (!str.empty() && (str.front() == 'z' || str.front() == 'Z')) {
                mode = MaskMode::Zero;
                str.remove_prefix(1);
                str = skip_whitespace(str);
                if (!str.empty() && str.front() == '}') {
                    str.remove_prefix(1);
                }
            }
        }
        return {{mask_reg, mode}, str};
    }
    
    constexpr std::pair<Operand, std::string_view> parse_operand(std::string_view str) {
        str = skip_whitespace(str);
        Operand op;
        std::string_view rem;
        
        if (!str.empty() && str.front() == '[') {
            auto mem_pair = parse_memory(str);
            op = Operand{mem_pair.first};
            rem = mem_pair.second;
        } else if (!str.empty() && str.front() == '@') {
            auto ph_pair = parse_placeholder(str);
            op = ph_pair.first;
            rem = ph_pair.second;
        } else if (!str.empty() && is_alpha(str.front())) {
            auto reg_pair = parse_register(str);
            op = reg_pair.first;
            rem = reg_pair.second;
        } else {
            auto imm_pair = parse_number(str);
            op = Operand{imm_pair.first};
            rem = imm_pair.second;
        }
        
        auto mask_result = parse_mask_decorator(rem);
        auto mask_info = mask_result.first;
        if (mask_info.first != Reg::NONE) {
            op.opmask = mask_info.first;
            op.mask_mode = mask_info.second;
        }
        return {op, mask_result.second};
    }
    
    constexpr VexType detect_vector_size(std::string_view instr) {
        bool has_mask = false;
        for (size_t i = 0; i < instr.size(); ++i) {
            if (instr[i] == '{' && i + 1 < instr.size() && 
                (instr[i + 1] == 'k' || instr[i + 1] == 'K')) {
                has_mask = true;
                break;
            }
        }
        for (size_t i = 0; i + 3 <= instr.size(); ++i) {
            char c0 = to_lower(instr[i]);
            char c1 = to_lower(instr[i+1]);
            char c2 = to_lower(instr[i+2]);
            if (c0 == 'z' && c1 == 'm' && c2 == 'm') return VexType::EVEX_512;
            if (c0 == 'y' && c1 == 'm' && c2 == 'm') return has_mask ? VexType::EVEX_256 : VexType::VEX_256;
            if (c0 == 'x' && c1 == 'm' && c2 == 'm') return has_mask ? VexType::EVEX_128 : VexType::VEX_128;
        }
        return VexType::NONE;
    }
}

constexpr bool fits_in_imm8(uint64_t imm) {
    int64_t s = static_cast<int64_t>(imm);
    return s >= -128 && s <= 127;
}

constexpr RegSize get_operand_size(const Operand* ops, size_t count) {
    // Special case: for shift/rotate instructions with CL register,
    // use the size of the destination operand (first operand)
    if (count >= 2 &&
        (ops[1].type == OperandType::Register || ops[1].type == OperandType::RegisterPlaceholder) &&
        ops[1].reg == Reg::CL) {
        // Return size of first operand (destination)
        if (ops[0].type == OperandType::Register || ops[0].type == OperandType::RegisterPlaceholder) {
            return ops[0].reg_size;
        }
        if (ops[0].type == OperandType::Memory || ops[0].type == OperandType::PointerPlaceholder) {
            return ops[0].reg_size;
        }
    }

    // Determine size from the first register operand found (skip CL)
    for (size_t i = 0; i < count; ++i) {
        if (ops[i].type == OperandType::Register || ops[i].type == OperandType::RegisterPlaceholder) {
            // Skip CL register when determining size
            if (ops[i].reg != Reg::CL) {
                return ops[i].reg_size;
            }
        }
    }

    // Check memory operands for their size
    for (size_t i = 0; i < count; ++i) {
        if (ops[i].type == OperandType::Memory || ops[i].type == OperandType::PointerPlaceholder) {
            return ops[i].reg_size;
        }
    }

    // Default to DWORD for operations without registers or memory
    return RegSize::DWORD;
}

constexpr InstructionEncoding make_instr(const char* mn, EncodingType t, uint8_t op, bool rw) {
    return {mn, t, 0, op, 0, 0, 0, rw, true, false, false, true, VexType::NONE, 0, false};
}

constexpr InstructionEncoding make_instr_reg(const char* mn, EncodingType t, uint8_t op, uint8_t reg, bool rw) {
    return {mn, t, 0, op, 0, 0, reg, rw, true, false, false, true, VexType::NONE, 0, false};
}

constexpr InstructionEncoding make_instr_2byte(const char* mn, EncodingType t, uint8_t op2, bool rw) {
    return {mn, t, 0, 0x0F, op2, 0, 0, rw, true, true, false, true, VexType::NONE, 0, false};
}

constexpr InstructionEncoding make_vex_instr(const char* mn, EncodingType t, uint8_t pp, uint8_t map,
    uint8_t op, VexType vex, bool w = false) {
    return { mn, t, pp, op, 0, 0, 0, false, true, false, false, true, vex, map, w };
}

constexpr InstructionEncoding make_vex_instr_reg(const char* mn, EncodingType t, uint8_t pp, uint8_t map,
    uint8_t op, uint8_t reg, VexType vex, bool w = false) {
    return { mn, t, pp, op, 0, 0, reg, false, true, false, false, true, vex, map, w };
}

constexpr InstructionEncoding make_evex_instr(const char* mn, EncodingType t, uint8_t pp, uint8_t map,
    uint8_t op, VexType vex, bool w = false) {
    return { mn, t, pp, op, 0, 0, 0, false, true, false, false, true, vex, map, w };
}

constexpr InstructionEncoding make_evex_instr_reg(const char* mn, EncodingType t, uint8_t pp, uint8_t map,
    uint8_t op, uint8_t reg, VexType vex, bool w = false) {
    return { mn, t, pp, op, 0, 0, reg, false, true, false, false, true, vex, map, w };
}

constexpr InstructionEncoding make_jmp_instr(const char* mn, uint8_t op_short, uint8_t op_near) {
    return {mn, EncodingType::JUMP_LABEL, 0, op_short, op_near, 0, 0, false, true, false, false, false, VexType::NONE, 0, false};
}

constexpr InstructionEncoding make_acc_instr(const char* mn, uint8_t op) {
    return {mn, EncodingType::ACC_IMM, 0, op, 0, 0, 0, true, false, false, false, false, VexType::NONE, 0, false};
}

constexpr std::array<InstructionEncoding, 4096> INSTRUCTION_TABLE = {{

    // mov
    make_instr("mov", EncodingType::REG_REG, 0x89, true),
    make_instr("mov", EncodingType::REG_MEM, 0x8B, true),
    make_instr("mov", EncodingType::MEM_REG, 0x89, true),
    make_instr("mov", EncodingType::REG_IMM, 0xB8, true),
    make_instr_reg("mov", EncodingType::MEM_IMM, 0xC7, 0, false),

    // movsx
    make_instr_2byte("movsx", EncodingType::REG_REG, 0xBE, true),
    make_instr_2byte("movsx", EncodingType::REG_MEM, 0xBE, true),

    // movsxd
    make_instr("movsxd", EncodingType::REG_REG, 0x63, true),
    make_instr("movsxd", EncodingType::REG_MEM, 0x63, true),

    // movzx
    make_instr_2byte("movzx", EncodingType::REG_REG, 0xB6, true),
    make_instr_2byte("movzx", EncodingType::REG_MEM, 0xB6, true),

    // CMOVcc instructions - Conditional move
    // note: doesn't support 8-bit registers, only 16/32/64-bit operands
    make_instr_2byte("cmove", EncodingType::REG_REG, 0x44, true),
    make_instr_2byte("cmove", EncodingType::REG_MEM, 0x44, true),
    make_instr_2byte("cmovz", EncodingType::REG_REG, 0x44, true),
    make_instr_2byte("cmovz", EncodingType::REG_MEM, 0x44, true),

    make_instr_2byte("cmovne", EncodingType::REG_REG, 0x45, true),
    make_instr_2byte("cmovne", EncodingType::REG_MEM, 0x45, true),
    make_instr_2byte("cmovnz", EncodingType::REG_REG, 0x45, true),
    make_instr_2byte("cmovnz", EncodingType::REG_MEM, 0x45, true),

    make_instr_2byte("cmovl", EncodingType::REG_REG, 0x4C, true),
    make_instr_2byte("cmovl", EncodingType::REG_MEM, 0x4C, true),
    make_instr_2byte("cmovnge", EncodingType::REG_REG, 0x4C, true),
    make_instr_2byte("cmovnge", EncodingType::REG_MEM, 0x4C, true),

    make_instr_2byte("cmovle", EncodingType::REG_REG, 0x4E, true),
    make_instr_2byte("cmovle", EncodingType::REG_MEM, 0x4E, true),
    make_instr_2byte("cmovng", EncodingType::REG_REG, 0x4E, true),
    make_instr_2byte("cmovng", EncodingType::REG_MEM, 0x4E, true),

    make_instr_2byte("cmovg", EncodingType::REG_REG, 0x4F, true),
    make_instr_2byte("cmovg", EncodingType::REG_MEM, 0x4F, true),
    make_instr_2byte("cmovnle", EncodingType::REG_REG, 0x4F, true),
    make_instr_2byte("cmovnle", EncodingType::REG_MEM, 0x4F, true),

    make_instr_2byte("cmovge", EncodingType::REG_REG, 0x4D, true),
    make_instr_2byte("cmovge", EncodingType::REG_MEM, 0x4D, true),
    make_instr_2byte("cmovnl", EncodingType::REG_REG, 0x4D, true),
    make_instr_2byte("cmovnl", EncodingType::REG_MEM, 0x4D, true),

    make_instr_2byte("cmovb", EncodingType::REG_REG, 0x42, true),
    make_instr_2byte("cmovb", EncodingType::REG_MEM, 0x42, true),
    make_instr_2byte("cmovnae", EncodingType::REG_REG, 0x42, true),
    make_instr_2byte("cmovnae", EncodingType::REG_MEM, 0x42, true),
    make_instr_2byte("cmovc", EncodingType::REG_REG, 0x42, true),
    make_instr_2byte("cmovc", EncodingType::REG_MEM, 0x42, true),

    make_instr_2byte("cmovbe", EncodingType::REG_REG, 0x46, true),
    make_instr_2byte("cmovbe", EncodingType::REG_MEM, 0x46, true),
    make_instr_2byte("cmovna", EncodingType::REG_REG, 0x46, true),
    make_instr_2byte("cmovna", EncodingType::REG_MEM, 0x46, true),

    make_instr_2byte("cmova", EncodingType::REG_REG, 0x47, true),
    make_instr_2byte("cmova", EncodingType::REG_MEM, 0x47, true),
    make_instr_2byte("cmovnbe", EncodingType::REG_REG, 0x47, true),
    make_instr_2byte("cmovnbe", EncodingType::REG_MEM, 0x47, true),

    make_instr_2byte("cmovae", EncodingType::REG_REG, 0x43, true),
    make_instr_2byte("cmovae", EncodingType::REG_MEM, 0x43, true),
    make_instr_2byte("cmovnb", EncodingType::REG_REG, 0x43, true),
    make_instr_2byte("cmovnb", EncodingType::REG_MEM, 0x43, true),
    make_instr_2byte("cmovnc", EncodingType::REG_REG, 0x43, true),
    make_instr_2byte("cmovnc", EncodingType::REG_MEM, 0x43, true),

    make_instr_2byte("cmovs", EncodingType::REG_REG, 0x48, true),
    make_instr_2byte("cmovs", EncodingType::REG_MEM, 0x48, true),
    make_instr_2byte("cmovns", EncodingType::REG_REG, 0x49, true),
    make_instr_2byte("cmovns", EncodingType::REG_MEM, 0x49, true),

    make_instr_2byte("cmovo", EncodingType::REG_REG, 0x40, true),
    make_instr_2byte("cmovo", EncodingType::REG_MEM, 0x40, true),
    make_instr_2byte("cmovno", EncodingType::REG_REG, 0x41, true),
    make_instr_2byte("cmovno", EncodingType::REG_MEM, 0x41, true),

    make_instr_2byte("cmovp", EncodingType::REG_REG, 0x4A, true),
    make_instr_2byte("cmovp", EncodingType::REG_MEM, 0x4A, true),
    make_instr_2byte("cmovpe", EncodingType::REG_REG, 0x4A, true),
    make_instr_2byte("cmovpe", EncodingType::REG_MEM, 0x4A, true),

    make_instr_2byte("cmovnp", EncodingType::REG_REG, 0x4B, true),
    make_instr_2byte("cmovnp", EncodingType::REG_MEM, 0x4B, true),
    make_instr_2byte("cmovpo", EncodingType::REG_REG, 0x4B, true),
    make_instr_2byte("cmovpo", EncodingType::REG_MEM, 0x4B, true),

    // add
    make_instr("add", EncodingType::REG_REG, 0x01, true),
    make_instr_reg("add", EncodingType::REG_IMM, 0x81, 0, true),
    make_instr_reg("add", EncodingType::REG_IMM8, 0x83, 0, true),
    make_acc_instr("add", 0x05),
    make_instr("add", EncodingType::REG_MEM, 0x03, true),
    make_instr("add", EncodingType::MEM_REG, 0x01, true),
    make_instr_reg("add", EncodingType::MEM_IMM, 0x81, 0, true),
    make_instr_reg("add", EncodingType::MEM_IMM8, 0x83, 0, true),

    // adc
    make_instr("adc", EncodingType::REG_REG, 0x11, true),
    make_instr_reg("adc", EncodingType::REG_IMM, 0x81, 2, true),
    make_instr_reg("adc", EncodingType::REG_IMM8, 0x83, 2, true),
    make_acc_instr("adc", 0x15),
    make_instr("adc", EncodingType::REG_MEM, 0x13, true),
    make_instr("adc", EncodingType::MEM_REG, 0x11, true),
    make_instr_reg("adc", EncodingType::MEM_IMM, 0x81, 2, true),
    make_instr_reg("adc", EncodingType::MEM_IMM8, 0x83, 2, true),

    // sub
    make_instr("sub", EncodingType::REG_REG, 0x29, true),
    make_instr_reg("sub", EncodingType::REG_IMM, 0x81, 5, true),
    make_instr_reg("sub", EncodingType::REG_IMM8, 0x83, 5, true),
    make_acc_instr("sub", 0x2D),
    make_instr("sub", EncodingType::REG_MEM, 0x2B, true),
    make_instr("sub", EncodingType::MEM_REG, 0x29, true),
    make_instr_reg("sub", EncodingType::MEM_IMM, 0x81, 5, true),
    make_instr_reg("sub", EncodingType::MEM_IMM8, 0x83, 5, true),

    // sbb
    make_instr("sbb", EncodingType::REG_REG, 0x19, true),
    make_instr_reg("sbb", EncodingType::REG_IMM, 0x81, 3, true),
    make_instr_reg("sbb", EncodingType::REG_IMM8, 0x83, 3, true),
    make_acc_instr("sbb", 0x1D),
    make_instr("sbb", EncodingType::REG_MEM, 0x1B, true),
    make_instr("sbb", EncodingType::MEM_REG, 0x19, true),
    make_instr_reg("sbb", EncodingType::MEM_IMM, 0x81, 3, true),
    make_instr_reg("sbb", EncodingType::MEM_IMM8, 0x83, 3, true),

    // imul
    make_instr_2byte("imul", EncodingType::REG_REG, 0xAF, true),
    make_instr_2byte("imul", EncodingType::REG_MEM, 0xAF, true),

    // One-operand form (like MUL)
    make_instr_reg("imul", EncodingType::SINGLE_REG, 0xF7, 5, true),
    make_instr_reg("imul", EncodingType::SINGLE_MEM, 0xF7, 5, true),

    // Two-operand with immediate (reg, imm)
    make_instr_reg("imul", EncodingType::REG_IMM, 0x69, 0, true),
    make_instr_reg("imul", EncodingType::REG_IMM8, 0x6B, 0, true),

    // Three-operand form (reg, reg, imm)
    make_instr_reg("imul", EncodingType::REG_REG_IMM, 0x69, 0, true),
    make_instr_reg("imul", EncodingType::REG_REG_IMM8, 0x6B, 0, true),

    // Three-operand form (reg, mem, imm)
    make_instr_reg("imul", EncodingType::REG_MEM_IMM, 0x69, 0, true),
    make_instr_reg("imul", EncodingType::REG_MEM_IMM8, 0x6B, 0, true),

    // mul
    make_instr_reg("mul", EncodingType::SINGLE_REG, 0xF7, 4, true),
    make_instr_reg("mul", EncodingType::SINGLE_MEM, 0xF7, 4, true),

    // div
    make_instr_reg("div", EncodingType::SINGLE_REG, 0xF7, 6, true),
    make_instr_reg("div", EncodingType::SINGLE_MEM, 0xF7, 6, true),

    // idiv
    make_instr_reg("idiv", EncodingType::SINGLE_REG, 0xF7, 7, true),
    make_instr_reg("idiv", EncodingType::SINGLE_MEM, 0xF7, 7, true),

    // and
    make_instr("and", EncodingType::REG_REG, 0x21, true),
    make_instr_reg("and", EncodingType::REG_IMM, 0x81, 4, true),
    make_instr_reg("and", EncodingType::REG_IMM8, 0x83, 4, true),
    make_acc_instr("and", 0x25),
    make_instr("and", EncodingType::REG_MEM, 0x23, true),
    make_instr("and", EncodingType::MEM_REG, 0x21, true),
    make_instr_reg("and", EncodingType::MEM_IMM, 0x81, 4, true),
    make_instr_reg("and", EncodingType::MEM_IMM8, 0x83, 4, true),

    // or
    make_instr("or", EncodingType::REG_REG, 0x09, true),
    make_instr_reg("or", EncodingType::REG_IMM, 0x81, 1, true),
    make_instr_reg("or", EncodingType::REG_IMM8, 0x83, 1, true),
    make_acc_instr("or", 0x0D),
    make_instr("or", EncodingType::REG_MEM, 0x0B, true),
    make_instr("or", EncodingType::MEM_REG, 0x09, true),
    make_instr_reg("or", EncodingType::MEM_IMM, 0x81, 1, true),
    make_instr_reg("or", EncodingType::MEM_IMM8, 0x83, 1, true),

    // xor
    make_instr("xor", EncodingType::REG_REG, 0x31, true),
    make_instr_reg("xor", EncodingType::REG_IMM, 0x81, 6, true),
    make_instr_reg("xor", EncodingType::REG_IMM8, 0x83, 6, true),
    make_acc_instr("xor", 0x35),
    make_instr("xor", EncodingType::REG_MEM, 0x33, true),
    make_instr("xor", EncodingType::MEM_REG, 0x31, true),
    make_instr_reg("xor", EncodingType::MEM_IMM, 0x81, 6, true),
    make_instr_reg("xor", EncodingType::MEM_IMM8, 0x83, 6, true),

    // cmp
    make_instr("cmp", EncodingType::REG_REG, 0x39, true),
    make_instr("cmp", EncodingType::REG_MEM, 0x3B, true),
    make_instr_reg("cmp", EncodingType::REG_IMM, 0x81, 7, true),
    make_instr_reg("cmp", EncodingType::REG_IMM8, 0x83, 7, true),
    make_acc_instr("cmp", 0x3D),
    make_instr("cmp", EncodingType::MEM_REG, 0x39, true),
    make_instr_reg("cmp", EncodingType::MEM_IMM, 0x81, 7, true),
    make_instr_reg("cmp", EncodingType::MEM_IMM8, 0x83, 7, true),

    // lea
    make_instr("lea", EncodingType::REG_MEM, 0x8D, true),

    // inc
    make_instr_reg("inc", EncodingType::SINGLE_REG, 0xFF, 0, true),
    make_instr_reg("inc", EncodingType::SINGLE_MEM, 0xFF, 0, true),

    // dec
    make_instr_reg("dec", EncodingType::SINGLE_REG, 0xFF, 1, true),
    make_instr_reg("dec", EncodingType::SINGLE_MEM, 0xFF, 1, true),

    // neg
    make_instr_reg("neg", EncodingType::SINGLE_REG, 0xF7, 3, true),
    make_instr_reg("neg", EncodingType::SINGLE_MEM, 0xF7, 3, true),

    // not
    make_instr_reg("not", EncodingType::SINGLE_REG, 0xF7, 2, true),
    make_instr_reg("not", EncodingType::SINGLE_MEM, 0xF7, 2, true),

    // SETcc instructions - Set byte on condition
    make_instr_2byte("sete", EncodingType::SINGLE_REG, 0x94, false),
    make_instr_2byte("sete", EncodingType::SINGLE_MEM, 0x94, false),
    make_instr_2byte("setz", EncodingType::SINGLE_REG, 0x94, false),
    make_instr_2byte("setz", EncodingType::SINGLE_MEM, 0x94, false),

    make_instr_2byte("setne", EncodingType::SINGLE_REG, 0x95, false),
    make_instr_2byte("setne", EncodingType::SINGLE_MEM, 0x95, false),
    make_instr_2byte("setnz", EncodingType::SINGLE_REG, 0x95, false),
    make_instr_2byte("setnz", EncodingType::SINGLE_MEM, 0x95, false),

    make_instr_2byte("setl", EncodingType::SINGLE_REG, 0x9C, false),
    make_instr_2byte("setl", EncodingType::SINGLE_MEM, 0x9C, false),
    make_instr_2byte("setnge", EncodingType::SINGLE_REG, 0x9C, false),
    make_instr_2byte("setnge", EncodingType::SINGLE_MEM, 0x9C, false),

    make_instr_2byte("setle", EncodingType::SINGLE_REG, 0x9E, false),
    make_instr_2byte("setle", EncodingType::SINGLE_MEM, 0x9E, false),
    make_instr_2byte("setng", EncodingType::SINGLE_REG, 0x9E, false),
    make_instr_2byte("setng", EncodingType::SINGLE_MEM, 0x9E, false),

    make_instr_2byte("setg", EncodingType::SINGLE_REG, 0x9F, false),
    make_instr_2byte("setg", EncodingType::SINGLE_MEM, 0x9F, false),
    make_instr_2byte("setnle", EncodingType::SINGLE_REG, 0x9F, false),
    make_instr_2byte("setnle", EncodingType::SINGLE_MEM, 0x9F, false),

    make_instr_2byte("setge", EncodingType::SINGLE_REG, 0x9D, false),
    make_instr_2byte("setge", EncodingType::SINGLE_MEM, 0x9D, false),
    make_instr_2byte("setnl", EncodingType::SINGLE_REG, 0x9D, false),
    make_instr_2byte("setnl", EncodingType::SINGLE_MEM, 0x9D, false),

    make_instr_2byte("setb", EncodingType::SINGLE_REG, 0x92, false),
    make_instr_2byte("setb", EncodingType::SINGLE_MEM, 0x92, false),
    make_instr_2byte("setnae", EncodingType::SINGLE_REG, 0x92, false),
    make_instr_2byte("setnae", EncodingType::SINGLE_MEM, 0x92, false),
    make_instr_2byte("setc", EncodingType::SINGLE_REG, 0x92, false),
    make_instr_2byte("setc", EncodingType::SINGLE_MEM, 0x92, false),

    make_instr_2byte("setbe", EncodingType::SINGLE_REG, 0x96, false),
    make_instr_2byte("setbe", EncodingType::SINGLE_MEM, 0x96, false),
    make_instr_2byte("setna", EncodingType::SINGLE_REG, 0x96, false),
    make_instr_2byte("setna", EncodingType::SINGLE_MEM, 0x96, false),

    make_instr_2byte("seta", EncodingType::SINGLE_REG, 0x97, false),
    make_instr_2byte("seta", EncodingType::SINGLE_MEM, 0x97, false),
    make_instr_2byte("setnbe", EncodingType::SINGLE_REG, 0x97, false),
    make_instr_2byte("setnbe", EncodingType::SINGLE_MEM, 0x97, false),

    make_instr_2byte("setae", EncodingType::SINGLE_REG, 0x93, false),
    make_instr_2byte("setae", EncodingType::SINGLE_MEM, 0x93, false),
    make_instr_2byte("setnb", EncodingType::SINGLE_REG, 0x93, false),
    make_instr_2byte("setnb", EncodingType::SINGLE_MEM, 0x93, false),
    make_instr_2byte("setnc", EncodingType::SINGLE_REG, 0x93, false),
    make_instr_2byte("setnc", EncodingType::SINGLE_MEM, 0x93, false),

    make_instr_2byte("sets", EncodingType::SINGLE_REG, 0x98, false),
    make_instr_2byte("sets", EncodingType::SINGLE_MEM, 0x98, false),
    make_instr_2byte("setns", EncodingType::SINGLE_REG, 0x99, false),
    make_instr_2byte("setns", EncodingType::SINGLE_MEM, 0x99, false),

    make_instr_2byte("seto", EncodingType::SINGLE_REG, 0x90, false),
    make_instr_2byte("seto", EncodingType::SINGLE_MEM, 0x90, false),
    make_instr_2byte("setno", EncodingType::SINGLE_REG, 0x91, false),
    make_instr_2byte("setno", EncodingType::SINGLE_MEM, 0x91, false),

    make_instr_2byte("setp", EncodingType::SINGLE_REG, 0x9A, false),
    make_instr_2byte("setp", EncodingType::SINGLE_MEM, 0x9A, false),
    make_instr_2byte("setpe", EncodingType::SINGLE_REG, 0x9A, false),
    make_instr_2byte("setpe", EncodingType::SINGLE_MEM, 0x9A, false),

    make_instr_2byte("setnp", EncodingType::SINGLE_REG, 0x9B, false),
    make_instr_2byte("setnp", EncodingType::SINGLE_MEM, 0x9B, false),
    make_instr_2byte("setpo", EncodingType::SINGLE_REG, 0x9B, false),
    make_instr_2byte("setpo", EncodingType::SINGLE_MEM, 0x9B, false),

    // shl - Shift left (SHL/SAL use same opcode)
    make_instr_reg("shl", EncodingType::REG_IMM8, 0xC1, 4, true),
    make_instr_reg("shl", EncodingType::MEM_IMM8, 0xC1, 4, true),
    // Shift by CL register
    make_instr_reg("shl", EncodingType::REG_CL, 0xD3, 4, true),
    make_instr_reg("shl", EncodingType::MEM_CL, 0xD3, 4, true),

    // sal
    make_instr_reg("sal", EncodingType::REG_IMM8, 0xC1, 4, true),
    make_instr_reg("sal", EncodingType::MEM_IMM8, 0xC1, 4, true),
    // Shift by CL register
    make_instr_reg("sal", EncodingType::REG_CL, 0xD3, 4, true),
    make_instr_reg("sal", EncodingType::MEM_CL, 0xD3, 4, true),

    // shr - Shift right logical
    make_instr_reg("shr", EncodingType::REG_IMM8, 0xC1, 5, true),
    make_instr_reg("shr", EncodingType::MEM_IMM8, 0xC1, 5, true),
    // Shift by CL register
    make_instr_reg("shr", EncodingType::REG_CL, 0xD3, 5, true),
    make_instr_reg("shr", EncodingType::MEM_CL, 0xD3, 5, true),

    // sar - Shift right arithmetic
    make_instr_reg("sar", EncodingType::REG_IMM8, 0xC1, 7, true),
    make_instr_reg("sar", EncodingType::MEM_IMM8, 0xC1, 7, true),
    // Shift by CL register
    make_instr_reg("sar", EncodingType::REG_CL, 0xD3, 7, true),
    make_instr_reg("sar", EncodingType::MEM_CL, 0xD3, 7, true),

    // bsr - Bit Scan Reverse (find most significant set bit)
    make_instr_2byte("bsr", EncodingType::REG_REG, 0xBD, true),
    make_instr_2byte("bsr", EncodingType::REG_MEM, 0xBD, true),

    // push
    make_instr("push", EncodingType::SINGLE_REG, 0x50, false),

    // pop
    make_instr("pop", EncodingType::SINGLE_REG, 0x58, false),

    // cwd/cdq/cqo - sign-extend accumulator into eax:edx
    make_instr("cwd", EncodingType::NO_OPERANDS, 0x99, false),
    make_instr("cdq", EncodingType::NO_OPERANDS, 0x99, false),
    make_instr("cqo", EncodingType::NO_OPERANDS, 0x99, false),

    // cbw/cwde/cdqe - sign-extend accumulator into accumulator
    make_instr("cbw", EncodingType::NO_OPERANDS, 0x98, false),
    make_instr("cwde", EncodingType::NO_OPERANDS, 0x98, false),
    make_instr("cdqe", EncodingType::NO_OPERANDS, 0x98, false),
        
    // ret
    make_instr("ret", EncodingType::NO_OPERANDS, 0xC3, false),

    // nop
    make_instr("nop", EncodingType::NO_OPERANDS, 0x90, false),


    make_jmp_instr("jmp", 0xEB, 0xE9),
    make_jmp_instr("je", 0x74, 0x84), make_jmp_instr("jz", 0x74, 0x84),
    make_jmp_instr("jne", 0x75, 0x85), make_jmp_instr("jnz", 0x75, 0x85),
    make_jmp_instr("jl", 0x7C, 0x8C), make_jmp_instr("jnge", 0x7C, 0x8C),
    make_jmp_instr("jle", 0x7E, 0x8E), make_jmp_instr("jng", 0x7E, 0x8E),
    make_jmp_instr("jg", 0x7F, 0x8F), make_jmp_instr("jnle", 0x7F, 0x8F),
    make_jmp_instr("jge", 0x7D, 0x8D), make_jmp_instr("jnl", 0x7D, 0x8D),
    make_jmp_instr("jb", 0x72, 0x82), make_jmp_instr("jnae", 0x72, 0x82), make_jmp_instr("jc", 0x72, 0x82),
    make_jmp_instr("jbe", 0x76, 0x86), make_jmp_instr("jna", 0x76, 0x86),
    make_jmp_instr("ja", 0x77, 0x87), make_jmp_instr("jnbe", 0x77, 0x87),
    make_jmp_instr("jae", 0x73, 0x83), make_jmp_instr("jnb", 0x73, 0x83), make_jmp_instr("jnc", 0x73, 0x83),
    make_jmp_instr("js", 0x78, 0x88), make_jmp_instr("jns", 0x79, 0x89),
    make_jmp_instr("jo", 0x70, 0x80), make_jmp_instr("jno", 0x71, 0x81),
    make_jmp_instr("jp", 0x7A, 0x8A), make_jmp_instr("jpe", 0x7A, 0x8A),
    make_jmp_instr("jnp", 0x7B, 0x8B), make_jmp_instr("jpo", 0x7B, 0x8B),

    // Indirect jump/call through memory (RIP-relative to pointer)
    make_instr_reg("jmp", EncodingType::JUMP_PTR, 0xFF, 4, false),
    make_instr_reg("call", EncodingType::CALL_PTR, 0xFF, 2, false),
    
    // Direct RIP-relative jump/call to absolute address
    make_instr("jmp", EncodingType::JUMP_ABS, 0xE9, false),
    make_instr("call", EncodingType::CALL_ABS, 0xE8, false),

    // movdqa - Move Aligned Packed Integer Values (LOADS - opcode 0x6F)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vmovdqa", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x01, 0x6F, VexType::VEX_128),
    make_vex_instr("vmovdqa", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x01, 0x6F, VexType::VEX_128),
    make_vex_instr("vmovdqa", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x01, 0x6F, VexType::VEX_256),
    make_vex_instr("vmovdqa", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x01, 0x6F, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vmovdqa32", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_128),
    make_evex_instr("vmovdqa32", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_128),
    make_evex_instr("vmovdqa32", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_256),
    make_evex_instr("vmovdqa32", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_256),
    make_evex_instr("vmovdqa32", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_512),
    make_evex_instr("vmovdqa32", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_512),

    make_evex_instr("vmovdqa64", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_128, true),
    make_evex_instr("vmovdqa64", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_128, true),
    make_evex_instr("vmovdqa64", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_256, true),
    make_evex_instr("vmovdqa64", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_256, true),
    make_evex_instr("vmovdqa64", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_512, true),
    make_evex_instr("vmovdqa64", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6F, VexType::EVEX_512, true),

    // movdqa - STORES (mem <- reg) - opcode 0x7F
    make_vex_instr("vmovdqa", EncodingType::VEX_MEM_REG, VEX_PREFIX_66, 0x01, 0x7F, VexType::VEX_128),
    make_vex_instr("vmovdqa", EncodingType::VEX_MEM_REG, VEX_PREFIX_66, 0x01, 0x7F, VexType::VEX_256),

    make_evex_instr("vmovdqa32", EncodingType::EVEX_MEM_REG, VEX_PREFIX_66, 0x01, 0x7F, VexType::EVEX_128),
    make_evex_instr("vmovdqa32", EncodingType::EVEX_MEM_REG, VEX_PREFIX_66, 0x01, 0x7F, VexType::EVEX_256),
    make_evex_instr("vmovdqa32", EncodingType::EVEX_MEM_REG, VEX_PREFIX_66, 0x01, 0x7F, VexType::EVEX_512),

    make_evex_instr("vmovdqa64", EncodingType::EVEX_MEM_REG, VEX_PREFIX_66, 0x01, 0x7F, VexType::EVEX_128, true),
    make_evex_instr("vmovdqa64", EncodingType::EVEX_MEM_REG, VEX_PREFIX_66, 0x01, 0x7F, VexType::EVEX_256, true),
    make_evex_instr("vmovdqa64", EncodingType::EVEX_MEM_REG, VEX_PREFIX_66, 0x01, 0x7F, VexType::EVEX_512, true),

    // vpaddb - Add Packed Byte Integers
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpaddb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFC, VexType::VEX_128),
    make_vex_instr("vpaddb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFC, VexType::VEX_128),
    make_vex_instr("vpaddb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFC, VexType::VEX_256),
    make_vex_instr("vpaddb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFC, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpaddb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFC, VexType::EVEX_128),
    make_evex_instr("vpaddb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFC, VexType::EVEX_128),
    make_evex_instr("vpaddb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFC, VexType::EVEX_256),
    make_evex_instr("vpaddb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFC, VexType::EVEX_256),
    make_evex_instr("vpaddb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFC, VexType::EVEX_512),
    make_evex_instr("vpaddb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFC, VexType::EVEX_512),

    // vpaddw - Add Packed Word Integers
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpaddw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFD, VexType::VEX_128),
    make_vex_instr("vpaddw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFD, VexType::VEX_128),
    make_vex_instr("vpaddw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFD, VexType::VEX_256),
    make_vex_instr("vpaddw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFD, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpaddw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFD, VexType::EVEX_128),
    make_evex_instr("vpaddw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFD, VexType::EVEX_128),
    make_evex_instr("vpaddw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFD, VexType::EVEX_256),
    make_evex_instr("vpaddw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFD, VexType::EVEX_256),
    make_evex_instr("vpaddw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFD, VexType::EVEX_512),
    make_evex_instr("vpaddw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFD, VexType::EVEX_512),

    // vpaddd - Add Packed Dword Integers
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpaddd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFE, VexType::VEX_128),
    make_vex_instr("vpaddd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFE, VexType::VEX_128),
    make_vex_instr("vpaddd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFE, VexType::VEX_256),
    make_vex_instr("vpaddd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFE, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpaddd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFE, VexType::EVEX_128),
    make_evex_instr("vpaddd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFE, VexType::EVEX_128),
    make_evex_instr("vpaddd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFE, VexType::EVEX_256),
    make_evex_instr("vpaddd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFE, VexType::EVEX_256),
    make_evex_instr("vpaddd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFE, VexType::EVEX_512),
    make_evex_instr("vpaddd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFE, VexType::EVEX_512),
        
    // vpaddq - Add Packed Qword Integers
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpaddq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD4, VexType::VEX_128),
    make_vex_instr("vpaddq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD4, VexType::VEX_128),
    make_vex_instr("vpaddq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD4, VexType::VEX_256),
    make_vex_instr("vpaddq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD4, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpaddq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD4, VexType::EVEX_128),
    make_evex_instr("vpaddq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD4, VexType::EVEX_128),
    make_evex_instr("vpaddq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD4, VexType::EVEX_256),
    make_evex_instr("vpaddq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD4, VexType::EVEX_256),
    make_evex_instr("vpaddq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD4, VexType::EVEX_512),
    make_evex_instr("vpaddq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD4, VexType::EVEX_512),

    // vpaddsb - Add Packed Signed Byte Integers with Signed Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpaddsb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEC, VexType::VEX_128),
    make_vex_instr("vpaddsb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEC, VexType::VEX_128),
    make_vex_instr("vpaddsb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEC, VexType::VEX_256),
    make_vex_instr("vpaddsb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEC, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpaddsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEC, VexType::EVEX_128),
    make_evex_instr("vpaddsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEC, VexType::EVEX_128),
    make_evex_instr("vpaddsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEC, VexType::EVEX_256),
    make_evex_instr("vpaddsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEC, VexType::EVEX_256),
    make_evex_instr("vpaddsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEC, VexType::EVEX_512),
    make_evex_instr("vpaddsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEC, VexType::EVEX_512),

    // vpaddsw - Add Packed Signed Word Integers with Signed Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpaddsw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xED, VexType::VEX_128),
    make_vex_instr("vpaddsw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xED, VexType::VEX_128),
    make_vex_instr("vpaddsw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xED, VexType::VEX_256),
    make_vex_instr("vpaddsw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xED, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpaddsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xED, VexType::EVEX_128),
    make_evex_instr("vpaddsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xED, VexType::EVEX_128),
    make_evex_instr("vpaddsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xED, VexType::EVEX_256),
    make_evex_instr("vpaddsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xED, VexType::EVEX_256),
    make_evex_instr("vpaddsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xED, VexType::EVEX_512),
    make_evex_instr("vpaddsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xED, VexType::EVEX_512),

    // vpaddusb - Add Packed Unsigned Byte Integers with Unsigned Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpaddusb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDC, VexType::VEX_128),
    make_vex_instr("vpaddusb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDC, VexType::VEX_128),
    make_vex_instr("vpaddusb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDC, VexType::VEX_256),
    make_vex_instr("vpaddusb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDC, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpaddusb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDC, VexType::EVEX_128),
    make_evex_instr("vpaddusb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDC, VexType::EVEX_128),
    make_evex_instr("vpaddusb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDC, VexType::EVEX_256),
    make_evex_instr("vpaddusb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDC, VexType::EVEX_256),
    make_evex_instr("vpaddusb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDC, VexType::EVEX_512),
    make_evex_instr("vpaddusb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDC, VexType::EVEX_512),

    // vpaddusw - Add Packed Unsigned Word Integers with Unsigned Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpaddusw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDD, VexType::VEX_128),
    make_vex_instr("vpaddusw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDD, VexType::VEX_128),
    make_vex_instr("vpaddusw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDD, VexType::VEX_256),
    make_vex_instr("vpaddusw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDD, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpaddusw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDD, VexType::EVEX_128),
    make_evex_instr("vpaddusw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDD, VexType::EVEX_128),
    make_evex_instr("vpaddusw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDD, VexType::EVEX_256),
    make_evex_instr("vpaddusw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDD, VexType::EVEX_256),
    make_evex_instr("vpaddusw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDD, VexType::EVEX_512),
    make_evex_instr("vpaddusw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDD, VexType::EVEX_512),

    // vpsubb - Packed Subtract Byte Integers
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsubb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF8, VexType::VEX_128),
    make_vex_instr("vpsubb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF8, VexType::VEX_128),
    make_vex_instr("vpsubb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF8, VexType::VEX_256),
    make_vex_instr("vpsubb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF8, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsubb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF8, VexType::EVEX_128),
    make_evex_instr("vpsubb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF8, VexType::EVEX_128),
    make_evex_instr("vpsubb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF8, VexType::EVEX_256),
    make_evex_instr("vpsubb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF8, VexType::EVEX_256),
    make_evex_instr("vpsubb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF8, VexType::EVEX_512),
    make_evex_instr("vpsubb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF8, VexType::EVEX_512),

    // vpsubw - Packed Subtract Word Integers
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsubw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF9, VexType::VEX_128),
    make_vex_instr("vpsubw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF9, VexType::VEX_128),
    make_vex_instr("vpsubw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF9, VexType::VEX_256),
    make_vex_instr("vpsubw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF9, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsubw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF9, VexType::EVEX_128),
    make_evex_instr("vpsubw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF9, VexType::EVEX_128),
    make_evex_instr("vpsubw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF9, VexType::EVEX_256),
    make_evex_instr("vpsubw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF9, VexType::EVEX_256),
    make_evex_instr("vpsubw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF9, VexType::EVEX_512),
    make_evex_instr("vpsubw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF9, VexType::EVEX_512),

    // vpsubd - Packed Subtract Dword Integers
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsubd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFA, VexType::VEX_128),
    make_vex_instr("vpsubd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFA, VexType::VEX_128),
    make_vex_instr("vpsubd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFA, VexType::VEX_256),
    make_vex_instr("vpsubd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFA, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsubd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFA, VexType::EVEX_128),
    make_evex_instr("vpsubd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFA, VexType::EVEX_128),
    make_evex_instr("vpsubd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFA, VexType::EVEX_256),
    make_evex_instr("vpsubd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFA, VexType::EVEX_256),
    make_evex_instr("vpsubd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFA, VexType::EVEX_512),
    make_evex_instr("vpsubd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFA, VexType::EVEX_512),

    // vpsubq - Packed Subtract Qword Integers
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsubq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFB, VexType::VEX_128),
    make_vex_instr("vpsubq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFB, VexType::VEX_128),
    make_vex_instr("vpsubq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFB, VexType::VEX_256),
    make_vex_instr("vpsubq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFB, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsubq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFB, VexType::EVEX_128),
    make_evex_instr("vpsubq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFB, VexType::EVEX_128),
    make_evex_instr("vpsubq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFB, VexType::EVEX_256),
    make_evex_instr("vpsubq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFB, VexType::EVEX_256),
    make_evex_instr("vpsubq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xFB, VexType::EVEX_512),
    make_evex_instr("vpsubq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xFB, VexType::EVEX_512),

    // vpsubsb - Packed Subtract Signed Byte Integers with Signed Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsubsb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE8, VexType::VEX_128),
    make_vex_instr("vpsubsb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE8, VexType::VEX_128),
    make_vex_instr("vpsubsb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE8, VexType::VEX_256),
    make_vex_instr("vpsubsb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE8, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsubsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE8, VexType::EVEX_128),
    make_evex_instr("vpsubsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE8, VexType::EVEX_128),
    make_evex_instr("vpsubsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE8, VexType::EVEX_256),
    make_evex_instr("vpsubsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE8, VexType::EVEX_256),
    make_evex_instr("vpsubsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE8, VexType::EVEX_512),
    make_evex_instr("vpsubsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE8, VexType::EVEX_512),

    // vpsubsw - Packed Subtract Signed Word Integers with Signed Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsubsw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE9, VexType::VEX_128),
    make_vex_instr("vpsubsw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE9, VexType::VEX_128),
    make_vex_instr("vpsubsw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE9, VexType::VEX_256),
    make_vex_instr("vpsubsw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE9, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsubsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE9, VexType::EVEX_128),
    make_evex_instr("vpsubsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE9, VexType::EVEX_128),
    make_evex_instr("vpsubsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE9, VexType::EVEX_256),
    make_evex_instr("vpsubsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE9, VexType::EVEX_256),
    make_evex_instr("vpsubsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE9, VexType::EVEX_512),
    make_evex_instr("vpsubsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE9, VexType::EVEX_512),

    // vpsubusb - Packed Subtract Unsigned Byte Integers with Unsigned Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsubusb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD8, VexType::VEX_128),
    make_vex_instr("vpsubusb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD8, VexType::VEX_128),
    make_vex_instr("vpsubusb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD8, VexType::VEX_256),
    make_vex_instr("vpsubusb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD8, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsubusb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD8, VexType::EVEX_128),
    make_evex_instr("vpsubusb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD8, VexType::EVEX_128),
    make_evex_instr("vpsubusb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD8, VexType::EVEX_256),
    make_evex_instr("vpsubusb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD8, VexType::EVEX_256),
    make_evex_instr("vpsubusb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD8, VexType::EVEX_512),
    make_evex_instr("vpsubusb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD8, VexType::EVEX_512),

    // vpsubusw - Packed Subtract Unsigned Word Integers with Unsigned Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsubusw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD9, VexType::VEX_128),
    make_vex_instr("vpsubusw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD9, VexType::VEX_128),
    make_vex_instr("vpsubusw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD9, VexType::VEX_256),
    make_vex_instr("vpsubusw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD9, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsubusw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD9, VexType::EVEX_128),
    make_evex_instr("vpsubusw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD9, VexType::EVEX_128),
    make_evex_instr("vpsubusw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD9, VexType::EVEX_256),
    make_evex_instr("vpsubusw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD9, VexType::EVEX_256),
    make_evex_instr("vpsubusw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD9, VexType::EVEX_512),
    make_evex_instr("vpsubusw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD9, VexType::EVEX_512),

    // vpmullw - Packed Multiply Low Signed/Unsigned Words
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmullw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD5, VexType::VEX_128),
    make_vex_instr("vpmullw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD5, VexType::VEX_128),
    make_vex_instr("vpmullw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD5, VexType::VEX_256),
    make_vex_instr("vpmullw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD5, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmullw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD5, VexType::EVEX_128),
    make_evex_instr("vpmullw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD5, VexType::EVEX_128),
    make_evex_instr("vpmullw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD5, VexType::EVEX_256),
    make_evex_instr("vpmullw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD5, VexType::EVEX_256),
    make_evex_instr("vpmullw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD5, VexType::EVEX_512),
    make_evex_instr("vpmullw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD5, VexType::EVEX_512),

    // vpmulld - Packed Multiply Low Signed/Unsigned Dwords
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmulld", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x40, VexType::VEX_128),
    make_vex_instr("vpmulld", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x40, VexType::VEX_128),
    make_vex_instr("vpmulld", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x40, VexType::VEX_256),
    make_vex_instr("vpmulld", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x40, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmulld", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_128),
    make_evex_instr("vpmulld", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_128),
    make_evex_instr("vpmulld", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_256),
    make_evex_instr("vpmulld", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_256),
    make_evex_instr("vpmulld", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_512),
    make_evex_instr("vpmulld", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_512),

    // vpmullq - Packed Multiply Low Signed/Unsigned Qwords (AVX-512 only)
    // EVEX versions (AVX-512)
    make_evex_instr("vpmullq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_128, true),
    make_evex_instr("vpmullq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_128, true),
    make_evex_instr("vpmullq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_256, true),
    make_evex_instr("vpmullq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_256, true),
    make_evex_instr("vpmullq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_512, true),
    make_evex_instr("vpmullq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x40, VexType::EVEX_512, true),

    // vpmulhw - Packed Multiply High Signed Words
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmulhw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE5, VexType::VEX_128),
    make_vex_instr("vpmulhw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE5, VexType::VEX_128),
    make_vex_instr("vpmulhw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE5, VexType::VEX_256),
    make_vex_instr("vpmulhw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE5, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmulhw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE5, VexType::EVEX_128),
    make_evex_instr("vpmulhw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE5, VexType::EVEX_128),
    make_evex_instr("vpmulhw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE5, VexType::EVEX_256),
    make_evex_instr("vpmulhw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE5, VexType::EVEX_256),
    make_evex_instr("vpmulhw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE5, VexType::EVEX_512),
    make_evex_instr("vpmulhw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE5, VexType::EVEX_512),

    // vpmulhuw - Packed Multiply High Unsigned Words
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmulhuw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE4, VexType::VEX_128),
    make_vex_instr("vpmulhuw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE4, VexType::VEX_128),
    make_vex_instr("vpmulhuw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE4, VexType::VEX_256),
    make_vex_instr("vpmulhuw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE4, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmulhuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE4, VexType::EVEX_128),
    make_evex_instr("vpmulhuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE4, VexType::EVEX_128),
    make_evex_instr("vpmulhuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE4, VexType::EVEX_256),
    make_evex_instr("vpmulhuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE4, VexType::EVEX_256),
    make_evex_instr("vpmulhuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE4, VexType::EVEX_512),
    make_evex_instr("vpmulhuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE4, VexType::EVEX_512),

    // vpmulhrsw - Packed Multiply High with Round and Scale Signed Words
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmulhrsw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0B, VexType::VEX_128),
    make_vex_instr("vpmulhrsw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0B, VexType::VEX_128),
    make_vex_instr("vpmulhrsw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0B, VexType::VEX_256),
    make_vex_instr("vpmulhrsw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0B, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmulhrsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0B, VexType::EVEX_128),
    make_evex_instr("vpmulhrsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0B, VexType::EVEX_128),
    make_evex_instr("vpmulhrsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0B, VexType::EVEX_256),
    make_evex_instr("vpmulhrsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0B, VexType::EVEX_256),
    make_evex_instr("vpmulhrsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0B, VexType::EVEX_512),
    make_evex_instr("vpmulhrsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0B, VexType::EVEX_512),

    // vpmuludq - Packed Multiply Unsigned Dword Integers to Qword
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmuludq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF4, VexType::VEX_128),
    make_vex_instr("vpmuludq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF4, VexType::VEX_128),
    make_vex_instr("vpmuludq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF4, VexType::VEX_256),
    make_vex_instr("vpmuludq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF4, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmuludq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF4, VexType::EVEX_128),
    make_evex_instr("vpmuludq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF4, VexType::EVEX_128),
    make_evex_instr("vpmuludq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF4, VexType::EVEX_256),
    make_evex_instr("vpmuludq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF4, VexType::EVEX_256),
    make_evex_instr("vpmuludq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF4, VexType::EVEX_512),
    make_evex_instr("vpmuludq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF4, VexType::EVEX_512),

    // vpmuldq - Packed Multiply Signed Dword Integers to Qword
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmuldq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x28, VexType::VEX_128),
    make_vex_instr("vpmuldq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x28, VexType::VEX_128),
    make_vex_instr("vpmuldq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x28, VexType::VEX_256),
    make_vex_instr("vpmuldq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x28, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmuldq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x28, VexType::EVEX_128),
    make_evex_instr("vpmuldq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x28, VexType::EVEX_128),
    make_evex_instr("vpmuldq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x28, VexType::EVEX_256),
    make_evex_instr("vpmuldq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x28, VexType::EVEX_256),
    make_evex_instr("vpmuldq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x28, VexType::EVEX_512),
    make_evex_instr("vpmuldq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x28, VexType::EVEX_512),

        
    // vpabsw - Packed Absolute Value Word
    // VEX versions (AVX/AVX2) - 2-operand
    make_vex_instr("vpabsw", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x1D, VexType::VEX_128),
    make_vex_instr("vpabsw", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x1D, VexType::VEX_128),
    make_vex_instr("vpabsw", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x1D, VexType::VEX_256),
    make_vex_instr("vpabsw", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x1D, VexType::VEX_256),

    // vpabsd - Packed Absolute Value Dword
    // VEX versions (AVX/AVX2) - 2-operand
    make_vex_instr("vpabsd", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x1E, VexType::VEX_128),
    make_vex_instr("vpabsd", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x1E, VexType::VEX_128),
    make_vex_instr("vpabsd", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x1E, VexType::VEX_256),
    make_vex_instr("vpabsd", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x1E, VexType::VEX_256),

    // EVEX versions (AVX-512) - 2-operand
    make_evex_instr("vpabsw", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x1D, VexType::EVEX_128),
    make_evex_instr("vpabsw", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x1D, VexType::EVEX_128),  // CHANGED
    make_evex_instr("vpabsw", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x1D, VexType::EVEX_256),
    make_evex_instr("vpabsw", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x1D, VexType::EVEX_256),  // CHANGED
    make_evex_instr("vpabsw", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x1D, VexType::EVEX_512),
    make_evex_instr("vpabsw", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x1D, VexType::EVEX_512),  // CHANGED

    make_evex_instr("vpabsd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x1E, VexType::EVEX_128),
    make_evex_instr("vpabsd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x1E, VexType::EVEX_128),  // CHANGED
    make_evex_instr("vpabsd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x1E, VexType::EVEX_256),
    make_evex_instr("vpabsd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x1E, VexType::EVEX_256),  // CHANGED
    make_evex_instr("vpabsd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x1E, VexType::EVEX_512),
    make_evex_instr("vpabsd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x1E, VexType::EVEX_512),  // CHANGED

    // vpmovsxbw - Packed Move with Sign Extension Byte to Word
    // VEX versions (AVX/AVX2) - 2-operand
    make_vex_instr("vpmovsxbw", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x20, VexType::VEX_128),
    make_vex_instr("vpmovsxbw", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x20, VexType::VEX_128),
    make_vex_instr("vpmovsxbw", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x20, VexType::VEX_256),
    make_vex_instr("vpmovsxbw", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x20, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmovsxbw", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x20, VexType::EVEX_128),
    make_evex_instr("vpmovsxbw", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x20, VexType::EVEX_128),
    make_evex_instr("vpmovsxbw", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x20, VexType::EVEX_256),
    make_evex_instr("vpmovsxbw", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x20, VexType::EVEX_256),
    make_evex_instr("vpmovsxbw", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x20, VexType::EVEX_512),
    make_evex_instr("vpmovsxbw", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x20, VexType::EVEX_512),

    // vpmovsxbd - Packed Move with Sign Extension Byte to Dword
    make_vex_instr("vpmovsxbd", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x21, VexType::VEX_128),
    make_vex_instr("vpmovsxbd", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x21, VexType::VEX_128),
    make_vex_instr("vpmovsxbd", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x21, VexType::VEX_256),
    make_vex_instr("vpmovsxbd", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x21, VexType::VEX_256),

    make_evex_instr("vpmovsxbd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x21, VexType::EVEX_128),
    make_evex_instr("vpmovsxbd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x21, VexType::EVEX_128),
    make_evex_instr("vpmovsxbd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x21, VexType::EVEX_256),
    make_evex_instr("vpmovsxbd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x21, VexType::EVEX_256),
    make_evex_instr("vpmovsxbd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x21, VexType::EVEX_512),
    make_evex_instr("vpmovsxbd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x21, VexType::EVEX_512),

    // vpmovsxbq - Packed Move with Sign Extension Byte to Qword
    make_vex_instr("vpmovsxbq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x22, VexType::VEX_128),
    make_vex_instr("vpmovsxbq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x22, VexType::VEX_128),
    make_vex_instr("vpmovsxbq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x22, VexType::VEX_256),
    make_vex_instr("vpmovsxbq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x22, VexType::VEX_256),

    make_evex_instr("vpmovsxbq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x22, VexType::EVEX_128),
    make_evex_instr("vpmovsxbq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x22, VexType::EVEX_128),
    make_evex_instr("vpmovsxbq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x22, VexType::EVEX_256),
    make_evex_instr("vpmovsxbq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x22, VexType::EVEX_256),
    make_evex_instr("vpmovsxbq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x22, VexType::EVEX_512),
    make_evex_instr("vpmovsxbq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x22, VexType::EVEX_512),

    // vpmovsxwd - Packed Move with Sign Extension Word to Dword
    make_vex_instr("vpmovsxwd", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x23, VexType::VEX_128),
    make_vex_instr("vpmovsxwd", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x23, VexType::VEX_128),
    make_vex_instr("vpmovsxwd", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x23, VexType::VEX_256),
    make_vex_instr("vpmovsxwd", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x23, VexType::VEX_256),

    make_evex_instr("vpmovsxwd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x23, VexType::EVEX_128),
    make_evex_instr("vpmovsxwd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x23, VexType::EVEX_128),
    make_evex_instr("vpmovsxwd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x23, VexType::EVEX_256),
    make_evex_instr("vpmovsxwd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x23, VexType::EVEX_256),
    make_evex_instr("vpmovsxwd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x23, VexType::EVEX_512),
    make_evex_instr("vpmovsxwd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x23, VexType::EVEX_512),

    // vpmovsxwq - Packed Move with Sign Extension Word to Qword
    make_vex_instr("vpmovsxwq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x24, VexType::VEX_128),
    make_vex_instr("vpmovsxwq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x24, VexType::VEX_128),
    make_vex_instr("vpmovsxwq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x24, VexType::VEX_256),
    make_vex_instr("vpmovsxwq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x24, VexType::VEX_256),

    make_evex_instr("vpmovsxwq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x24, VexType::EVEX_128),
    make_evex_instr("vpmovsxwq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x24, VexType::EVEX_128),
    make_evex_instr("vpmovsxwq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x24, VexType::EVEX_256),
    make_evex_instr("vpmovsxwq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x24, VexType::EVEX_256),
    make_evex_instr("vpmovsxwq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x24, VexType::EVEX_512),
    make_evex_instr("vpmovsxwq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x24, VexType::EVEX_512),

    // vpmovsxdq - Packed Move with Sign Extension Dword to Qword
    make_vex_instr("vpmovsxdq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x25, VexType::VEX_128),
    make_vex_instr("vpmovsxdq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x25, VexType::VEX_128),
    make_vex_instr("vpmovsxdq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x25, VexType::VEX_256),
    make_vex_instr("vpmovsxdq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x25, VexType::VEX_256),

    make_evex_instr("vpmovsxdq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x25, VexType::EVEX_128),
    make_evex_instr("vpmovsxdq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x25, VexType::EVEX_128),
    make_evex_instr("vpmovsxdq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x25, VexType::EVEX_256),
    make_evex_instr("vpmovsxdq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x25, VexType::EVEX_256),
    make_evex_instr("vpmovsxdq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x25, VexType::EVEX_512),
    make_evex_instr("vpmovsxdq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x25, VexType::EVEX_512),

    // vpmovzxbw - Packed Move with Zero Extension Byte to Word
    make_vex_instr("vpmovzxbw", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x30, VexType::VEX_128),
    make_vex_instr("vpmovzxbw", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x30, VexType::VEX_128),
    make_vex_instr("vpmovzxbw", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x30, VexType::VEX_256),
    make_vex_instr("vpmovzxbw", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x30, VexType::VEX_256),

    make_evex_instr("vpmovzxbw", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x30, VexType::EVEX_128),
    make_evex_instr("vpmovzxbw", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x30, VexType::EVEX_128),
    make_evex_instr("vpmovzxbw", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x30, VexType::EVEX_256),
    make_evex_instr("vpmovzxbw", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x30, VexType::EVEX_256),
    make_evex_instr("vpmovzxbw", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x30, VexType::EVEX_512),
    make_evex_instr("vpmovzxbw", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x30, VexType::EVEX_512),

    // vpmovzxbd - Packed Move with Zero Extension Byte to Dword
    make_vex_instr("vpmovzxbd", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x31, VexType::VEX_128),
    make_vex_instr("vpmovzxbd", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x31, VexType::VEX_128),
    make_vex_instr("vpmovzxbd", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x31, VexType::VEX_256),
    make_vex_instr("vpmovzxbd", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x31, VexType::VEX_256),

    make_evex_instr("vpmovzxbd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x31, VexType::EVEX_128),
    make_evex_instr("vpmovzxbd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x31, VexType::EVEX_128),
    make_evex_instr("vpmovzxbd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x31, VexType::EVEX_256),
    make_evex_instr("vpmovzxbd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x31, VexType::EVEX_256),
    make_evex_instr("vpmovzxbd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x31, VexType::EVEX_512),
    make_evex_instr("vpmovzxbd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x31, VexType::EVEX_512),

    // vpmovzxbq - Packed Move with Zero Extension Byte to Qword
    make_vex_instr("vpmovzxbq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x32, VexType::VEX_128),
    make_vex_instr("vpmovzxbq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x32, VexType::VEX_128),
    make_vex_instr("vpmovzxbq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x32, VexType::VEX_256),
    make_vex_instr("vpmovzxbq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x32, VexType::VEX_256),

    make_evex_instr("vpmovzxbq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x32, VexType::EVEX_128),
    make_evex_instr("vpmovzxbq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x32, VexType::EVEX_128),
    make_evex_instr("vpmovzxbq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x32, VexType::EVEX_256),
    make_evex_instr("vpmovzxbq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x32, VexType::EVEX_256),
    make_evex_instr("vpmovzxbq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x32, VexType::EVEX_512),
    make_evex_instr("vpmovzxbq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x32, VexType::EVEX_512),

    // vpmovzxwd - Packed Move with Zero Extension Word to Dword
    make_vex_instr("vpmovzxwd", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x33, VexType::VEX_128),
    make_vex_instr("vpmovzxwd", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x33, VexType::VEX_128),
    make_vex_instr("vpmovzxwd", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x33, VexType::VEX_256),
    make_vex_instr("vpmovzxwd", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x33, VexType::VEX_256),

    make_evex_instr("vpmovzxwd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x33, VexType::EVEX_128),
    make_evex_instr("vpmovzxwd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x33, VexType::EVEX_128),
    make_evex_instr("vpmovzxwd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x33, VexType::EVEX_256),
    make_evex_instr("vpmovzxwd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x33, VexType::EVEX_256),
    make_evex_instr("vpmovzxwd", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x33, VexType::EVEX_512),
    make_evex_instr("vpmovzxwd", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x33, VexType::EVEX_512),

    // vpmovzxwq - Packed Move with Zero Extension Word to Qword
    make_vex_instr("vpmovzxwq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x34, VexType::VEX_128),
    make_vex_instr("vpmovzxwq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x34, VexType::VEX_128),
    make_vex_instr("vpmovzxwq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x34, VexType::VEX_256),
    make_vex_instr("vpmovzxwq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x34, VexType::VEX_256),

    make_evex_instr("vpmovzxwq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x34, VexType::EVEX_128),
    make_evex_instr("vpmovzxwq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x34, VexType::EVEX_128),
    make_evex_instr("vpmovzxwq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x34, VexType::EVEX_256),
    make_evex_instr("vpmovzxwq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x34, VexType::EVEX_256),
    make_evex_instr("vpmovzxwq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x34, VexType::EVEX_512),
    make_evex_instr("vpmovzxwq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x34, VexType::EVEX_512),

    // vpmovzxdq - Packed Move with Zero Extension Dword to Qword
    make_vex_instr("vpmovzxdq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x35, VexType::VEX_128),
    make_vex_instr("vpmovzxdq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x35, VexType::VEX_128),
    make_vex_instr("vpmovzxdq", EncodingType::VEX_REG_REG, VEX_PREFIX_66, 0x02, 0x35, VexType::VEX_256),
    make_vex_instr("vpmovzxdq", EncodingType::VEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x35, VexType::VEX_256),

    make_evex_instr("vpmovzxdq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x35, VexType::EVEX_128),
    make_evex_instr("vpmovzxdq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x35, VexType::EVEX_128),
    make_evex_instr("vpmovzxdq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x35, VexType::EVEX_256),
    make_evex_instr("vpmovzxdq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x35, VexType::EVEX_256),
    make_evex_instr("vpmovzxdq", EncodingType::EVEX_REG_REG, VEX_PREFIX_66, 0x02, 0x35, VexType::EVEX_512),
    make_evex_instr("vpmovzxdq", EncodingType::EVEX_REG_MEM, VEX_PREFIX_66, 0x02, 0x35, VexType::EVEX_512),

    // vpand - Packed Bitwise AND (3-operand)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpand", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDB, VexType::VEX_128),
    make_vex_instr("vpand", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDB, VexType::VEX_128),
    make_vex_instr("vpand", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDB, VexType::VEX_256),
    make_vex_instr("vpand", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDB, VexType::VEX_256),

    // EVEX versions (AVX-512) - uses vpandd for 32-bit and vpandq for 64-bit
    make_evex_instr("vpandd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_128),
    make_evex_instr("vpandd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_128),
    make_evex_instr("vpandd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_256),
    make_evex_instr("vpandd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_256),
    make_evex_instr("vpandd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_512),
    make_evex_instr("vpandd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_512),

    make_evex_instr("vpandq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_128, true),
    make_evex_instr("vpandq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_128, true),
    make_evex_instr("vpandq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_256, true),
    make_evex_instr("vpandq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_256, true),
    make_evex_instr("vpandq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_512, true),
    make_evex_instr("vpandq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDB, VexType::EVEX_512, true),

    // vpor - Packed Bitwise OR (3-operand)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpor", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEB, VexType::VEX_128),
    make_vex_instr("vpor", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEB, VexType::VEX_128),
    make_vex_instr("vpor", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEB, VexType::VEX_256),
    make_vex_instr("vpor", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEB, VexType::VEX_256),

    // EVEX versions (AVX-512) - uses vpord for 32-bit and vporq for 64-bit
    make_evex_instr("vpord", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_128),
    make_evex_instr("vpord", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_128),
    make_evex_instr("vpord", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_256),
    make_evex_instr("vpord", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_256),
    make_evex_instr("vpord", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_512),
    make_evex_instr("vpord", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_512),

    make_evex_instr("vporq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_128, true),
    make_evex_instr("vporq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_128, true),
    make_evex_instr("vporq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_256, true),
    make_evex_instr("vporq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_256, true),
    make_evex_instr("vporq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_512, true),
    make_evex_instr("vporq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEB, VexType::EVEX_512, true),

    // vpxor - Packed Bitwise XOR (3-operand)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpxor", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEF, VexType::VEX_128),
    make_vex_instr("vpxor", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEF, VexType::VEX_128),
    make_vex_instr("vpxor", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEF, VexType::VEX_256),
    make_vex_instr("vpxor", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEF, VexType::VEX_256),

    // EVEX versions (AVX-512) - uses vpxord for 32-bit and vpxorq for 64-bit
    make_evex_instr("vpxord", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_128),
    make_evex_instr("vpxord", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_128),
    make_evex_instr("vpxord", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_256),
    make_evex_instr("vpxord", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_256),
    make_evex_instr("vpxord", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_512),
    make_evex_instr("vpxord", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_512),

    make_evex_instr("vpxorq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_128, true),
    make_evex_instr("vpxorq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_128, true),
    make_evex_instr("vpxorq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_256, true),
    make_evex_instr("vpxorq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_256, true),
    make_evex_instr("vpxorq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_512, true),
    make_evex_instr("vpxorq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEF, VexType::EVEX_512, true),

    // ========================================================================
    // PCMP Instructions - Packed Compare
    // ========================================================================

    // vpcmpeqb - Compare Packed Byte Data for Equality
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpcmpeqb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x74, VexType::VEX_128),
    make_vex_instr("vpcmpeqb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x74, VexType::VEX_128),
    make_vex_instr("vpcmpeqb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x74, VexType::VEX_256),
    make_vex_instr("vpcmpeqb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x74, VexType::VEX_256),

    // EVEX versions (AVX-512) - writes to mask register
    make_evex_instr("vpcmpeqb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x74, VexType::EVEX_128),
    make_evex_instr("vpcmpeqb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x74, VexType::EVEX_128),
    make_evex_instr("vpcmpeqb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x74, VexType::EVEX_256),
    make_evex_instr("vpcmpeqb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x74, VexType::EVEX_256),
    make_evex_instr("vpcmpeqb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x74, VexType::EVEX_512),
    make_evex_instr("vpcmpeqb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x74, VexType::EVEX_512),

    // vpcmpeqw - Compare Packed Word Data for Equality
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpcmpeqw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x75, VexType::VEX_128),
    make_vex_instr("vpcmpeqw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x75, VexType::VEX_128),
    make_vex_instr("vpcmpeqw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x75, VexType::VEX_256),
    make_vex_instr("vpcmpeqw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x75, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpcmpeqw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x75, VexType::EVEX_128),
    make_evex_instr("vpcmpeqw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x75, VexType::EVEX_128),
    make_evex_instr("vpcmpeqw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x75, VexType::EVEX_256),
    make_evex_instr("vpcmpeqw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x75, VexType::EVEX_256),
    make_evex_instr("vpcmpeqw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x75, VexType::EVEX_512),
    make_evex_instr("vpcmpeqw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x75, VexType::EVEX_512),

    // vpcmpeqd - Compare Packed Dword Data for Equality
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpcmpeqd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x76, VexType::VEX_128),
    make_vex_instr("vpcmpeqd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x76, VexType::VEX_128),
    make_vex_instr("vpcmpeqd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x76, VexType::VEX_256),
    make_vex_instr("vpcmpeqd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x76, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpcmpeqd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x76, VexType::EVEX_128),
    make_evex_instr("vpcmpeqd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x76, VexType::EVEX_128),
    make_evex_instr("vpcmpeqd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x76, VexType::EVEX_256),
    make_evex_instr("vpcmpeqd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x76, VexType::EVEX_256),
    make_evex_instr("vpcmpeqd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x76, VexType::EVEX_512),
    make_evex_instr("vpcmpeqd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x76, VexType::EVEX_512),

    // vpcmpeqq - Compare Packed Qword Data for Equality
    // VEX versions (AVX/AVX2) - uses map 0x02 (0F 38)
    make_vex_instr("vpcmpeqq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x29, VexType::VEX_128),
    make_vex_instr("vpcmpeqq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x29, VexType::VEX_128),
    make_vex_instr("vpcmpeqq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x29, VexType::VEX_256),
    make_vex_instr("vpcmpeqq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x29, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpcmpeqq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x29, VexType::EVEX_128, true),
    make_evex_instr("vpcmpeqq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x29, VexType::EVEX_128, true),
    make_evex_instr("vpcmpeqq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x29, VexType::EVEX_256, true),
    make_evex_instr("vpcmpeqq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x29, VexType::EVEX_256, true),
    make_evex_instr("vpcmpeqq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x29, VexType::EVEX_512, true),
    make_evex_instr("vpcmpeqq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x29, VexType::EVEX_512, true),

    // vpcmpgtb - Compare Packed Signed Byte Integers for Greater Than
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpcmpgtb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x64, VexType::VEX_128),
    make_vex_instr("vpcmpgtb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x64, VexType::VEX_128),
    make_vex_instr("vpcmpgtb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x64, VexType::VEX_256),
    make_vex_instr("vpcmpgtb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x64, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpcmpgtb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x64, VexType::EVEX_128),
    make_evex_instr("vpcmpgtb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x64, VexType::EVEX_128),
    make_evex_instr("vpcmpgtb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x64, VexType::EVEX_256),
    make_evex_instr("vpcmpgtb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x64, VexType::EVEX_256),
    make_evex_instr("vpcmpgtb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x64, VexType::EVEX_512),
    make_evex_instr("vpcmpgtb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x64, VexType::EVEX_512),

    // vpcmpgtw - Compare Packed Signed Word Integers for Greater Than
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpcmpgtw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x65, VexType::VEX_128),
    make_vex_instr("vpcmpgtw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x65, VexType::VEX_128),
    make_vex_instr("vpcmpgtw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x65, VexType::VEX_256),
    make_vex_instr("vpcmpgtw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x65, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpcmpgtw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x65, VexType::EVEX_128),
    make_evex_instr("vpcmpgtw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x65, VexType::EVEX_128),
    make_evex_instr("vpcmpgtw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x65, VexType::EVEX_256),
    make_evex_instr("vpcmpgtw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x65, VexType::EVEX_256),
    make_evex_instr("vpcmpgtw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x65, VexType::EVEX_512),
    make_evex_instr("vpcmpgtw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x65, VexType::EVEX_512),

    // vpcmpgtd - Compare Packed Signed Dword Integers for Greater Than
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpcmpgtd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x66, VexType::VEX_128),
    make_vex_instr("vpcmpgtd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x66, VexType::VEX_128),
    make_vex_instr("vpcmpgtd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x66, VexType::VEX_256),
    make_vex_instr("vpcmpgtd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x66, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpcmpgtd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x66, VexType::EVEX_128),
    make_evex_instr("vpcmpgtd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x66, VexType::EVEX_128),
    make_evex_instr("vpcmpgtd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x66, VexType::EVEX_256),
    make_evex_instr("vpcmpgtd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x66, VexType::EVEX_256),
    make_evex_instr("vpcmpgtd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x66, VexType::EVEX_512),
    make_evex_instr("vpcmpgtd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x66, VexType::EVEX_512),

    // vpcmpgtq - Compare Packed Signed Qword Integers for Greater Than
    // VEX versions (AVX/AVX2) - uses map 0x02 (0F 38)
    make_vex_instr("vpcmpgtq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x37, VexType::VEX_128),
    make_vex_instr("vpcmpgtq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x37, VexType::VEX_128),
    make_vex_instr("vpcmpgtq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x37, VexType::VEX_256),
    make_vex_instr("vpcmpgtq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x37, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpcmpgtq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x37, VexType::EVEX_128, true),
    make_evex_instr("vpcmpgtq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x37, VexType::EVEX_128, true),
    make_evex_instr("vpcmpgtq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x37, VexType::EVEX_256, true),
    make_evex_instr("vpcmpgtq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x37, VexType::EVEX_256, true),
    make_evex_instr("vpcmpgtq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x37, VexType::EVEX_512, true),
    make_evex_instr("vpcmpgtq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x37, VexType::EVEX_512, true),

    // AVX-512 Extended Compare Instructions with Immediate (3-operand + imm8)
    // These use a different encoding and support all 8 comparison predicates

    // vpcmpb/vpcmpub - Compare Packed Byte Integers (signed/unsigned)
    make_evex_instr("vpcmpb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_128),
    make_evex_instr("vpcmpb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_128),
    make_evex_instr("vpcmpb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_256),
    make_evex_instr("vpcmpb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_256),
    make_evex_instr("vpcmpb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_512),
    make_evex_instr("vpcmpb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_512),

    make_evex_instr("vpcmpub", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_128),
    make_evex_instr("vpcmpub", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_128),
    make_evex_instr("vpcmpub", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_256),
    make_evex_instr("vpcmpub", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_256),
    make_evex_instr("vpcmpub", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_512),
    make_evex_instr("vpcmpub", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_512),

    // vpcmpw/vpcmpuw - Compare Packed Word Integers (signed/unsigned)
    make_evex_instr("vpcmpw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_128),
    make_evex_instr("vpcmpw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_128),
    make_evex_instr("vpcmpw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_256),
    make_evex_instr("vpcmpw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_256),
    make_evex_instr("vpcmpw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_512),
    make_evex_instr("vpcmpw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3F, VexType::EVEX_512),

    make_evex_instr("vpcmpuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_128),
    make_evex_instr("vpcmpuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_128),
    make_evex_instr("vpcmpuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_256),
    make_evex_instr("vpcmpuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_256),
    make_evex_instr("vpcmpuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_512),
    make_evex_instr("vpcmpuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x3E, VexType::EVEX_512),

    // vpcmpd/vpcmpud - Compare Packed Dword Integers (signed/unsigned)
    make_evex_instr("vpcmpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_128),
    make_evex_instr("vpcmpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_128),
    make_evex_instr("vpcmpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_256),
    make_evex_instr("vpcmpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_256),
    make_evex_instr("vpcmpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_512),
    make_evex_instr("vpcmpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_512),

    make_evex_instr("vpcmpud", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_128),
    make_evex_instr("vpcmpud", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_128),
    make_evex_instr("vpcmpud", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_256),
    make_evex_instr("vpcmpud", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_256),
    make_evex_instr("vpcmpud", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_512),
    make_evex_instr("vpcmpud", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_512),

    // vpcmpq/vpcmpuq - Compare Packed Qword Integers (signed/unsigned)
    make_evex_instr("vpcmpq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_128, true),
    make_evex_instr("vpcmpq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_128, true),
    make_evex_instr("vpcmpq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_256, true),
    make_evex_instr("vpcmpq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_256, true),
    make_evex_instr("vpcmpq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_512, true),
    make_evex_instr("vpcmpq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1F, VexType::EVEX_512, true),

    make_evex_instr("vpcmpuq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_128, true),
    make_evex_instr("vpcmpuq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_128, true),
    make_evex_instr("vpcmpuq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_256, true),
    make_evex_instr("vpcmpuq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_256, true),
    make_evex_instr("vpcmpuq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_512, true),
    make_evex_instr("vpcmpuq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x1E, VexType::EVEX_512, true),

    // ========================================================================
    // Packed Shift Instructions - Complete with Register and Immediate forms
    // ========================================================================

    // vpsllw - Packed Shift Left Logical Word
    // VEX versions (AVX/AVX2) - 3-operand with XMM shift count
    make_vex_instr("vpsllw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF1, VexType::VEX_128),
    make_vex_instr("vpsllw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF1, VexType::VEX_128),
    make_vex_instr("vpsllw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF1, VexType::VEX_256),
    make_vex_instr("vpsllw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF1, VexType::VEX_256),

    // VEX immediate versions (reg field = 6)
    make_vex_instr_reg("vpsllw", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 6, VexType::VEX_128),
    make_vex_instr_reg("vpsllw", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 6, VexType::VEX_128),
    make_vex_instr_reg("vpsllw", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 6, VexType::VEX_256),
    make_vex_instr_reg("vpsllw", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 6, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsllw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF1, VexType::EVEX_128),
    make_evex_instr("vpsllw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF1, VexType::EVEX_128),
    make_evex_instr("vpsllw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF1, VexType::EVEX_256),
    make_evex_instr("vpsllw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF1, VexType::EVEX_256),
    make_evex_instr("vpsllw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF1, VexType::EVEX_512),
    make_evex_instr("vpsllw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF1, VexType::EVEX_512),

    // EVEX immediate versions
    make_evex_instr_reg("vpsllw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 6, VexType::EVEX_128),
    make_evex_instr_reg("vpsllw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 6, VexType::EVEX_128),
    make_evex_instr_reg("vpsllw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 6, VexType::EVEX_256),
    make_evex_instr_reg("vpsllw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 6, VexType::EVEX_256),
    make_evex_instr_reg("vpsllw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 6, VexType::EVEX_512),
    make_evex_instr_reg("vpsllw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 6, VexType::EVEX_512),

    // vpslld - Packed Shift Left Logical Dword
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpslld", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF2, VexType::VEX_128),
    make_vex_instr("vpslld", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF2, VexType::VEX_128),
    make_vex_instr("vpslld", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF2, VexType::VEX_256),
    make_vex_instr("vpslld", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF2, VexType::VEX_256),

    // VEX immediate versions (reg field = 6)
    make_vex_instr_reg("vpslld", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 6, VexType::VEX_128),
    make_vex_instr_reg("vpslld", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 6, VexType::VEX_128),
    make_vex_instr_reg("vpslld", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 6, VexType::VEX_256),
    make_vex_instr_reg("vpslld", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 6, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpslld", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF2, VexType::EVEX_128),
    make_evex_instr("vpslld", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF2, VexType::EVEX_128),
    make_evex_instr("vpslld", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF2, VexType::EVEX_256),
    make_evex_instr("vpslld", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF2, VexType::EVEX_256),
    make_evex_instr("vpslld", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF2, VexType::EVEX_512),
    make_evex_instr("vpslld", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF2, VexType::EVEX_512),

    // EVEX immediate versions
    make_evex_instr_reg("vpslld", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 6, VexType::EVEX_128),
    make_evex_instr_reg("vpslld", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 6, VexType::EVEX_128),
    make_evex_instr_reg("vpslld", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 6, VexType::EVEX_256),
    make_evex_instr_reg("vpslld", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 6, VexType::EVEX_256),
    make_evex_instr_reg("vpslld", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 6, VexType::EVEX_512),
    make_evex_instr_reg("vpslld", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 6, VexType::EVEX_512),

    // vpsllq - Packed Shift Left Logical Qword
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsllq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF3, VexType::VEX_128),
    make_vex_instr("vpsllq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF3, VexType::VEX_128),
    make_vex_instr("vpsllq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF3, VexType::VEX_256),
    make_vex_instr("vpsllq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF3, VexType::VEX_256),

    // VEX immediate versions (reg field = 6)
    make_vex_instr_reg("vpsllq", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 6, VexType::VEX_128),
    make_vex_instr_reg("vpsllq", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 6, VexType::VEX_128),
    make_vex_instr_reg("vpsllq", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 6, VexType::VEX_256),
    make_vex_instr_reg("vpsllq", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 6, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsllq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF3, VexType::EVEX_128, true),
    make_evex_instr("vpsllq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF3, VexType::EVEX_128, true),
    make_evex_instr("vpsllq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF3, VexType::EVEX_256, true),
    make_evex_instr("vpsllq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF3, VexType::EVEX_256, true),
    make_evex_instr("vpsllq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xF3, VexType::EVEX_512, true),
    make_evex_instr("vpsllq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xF3, VexType::EVEX_512, true),

    // EVEX immediate versions
    make_evex_instr_reg("vpsllq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 6, VexType::EVEX_128, true),
    make_evex_instr_reg("vpsllq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 6, VexType::EVEX_128, true),
    make_evex_instr_reg("vpsllq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 6, VexType::EVEX_256, true),
    make_evex_instr_reg("vpsllq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 6, VexType::EVEX_256, true),
    make_evex_instr_reg("vpsllq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 6, VexType::EVEX_512, true),
    make_evex_instr_reg("vpsllq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 6, VexType::EVEX_512, true),

    // vpsrlw - Packed Shift Right Logical Word
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsrlw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD1, VexType::VEX_128),
    make_vex_instr("vpsrlw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD1, VexType::VEX_128),
    make_vex_instr("vpsrlw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD1, VexType::VEX_256),
    make_vex_instr("vpsrlw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD1, VexType::VEX_256),

    // VEX immediate versions (reg field = 2)
    make_vex_instr_reg("vpsrlw", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 2, VexType::VEX_128),
    make_vex_instr_reg("vpsrlw", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 2, VexType::VEX_128),
    make_vex_instr_reg("vpsrlw", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 2, VexType::VEX_256),
    make_vex_instr_reg("vpsrlw", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 2, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsrlw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD1, VexType::EVEX_128),
    make_evex_instr("vpsrlw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD1, VexType::EVEX_128),
    make_evex_instr("vpsrlw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD1, VexType::EVEX_256),
    make_evex_instr("vpsrlw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD1, VexType::EVEX_256),
    make_evex_instr("vpsrlw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD1, VexType::EVEX_512),
    make_evex_instr("vpsrlw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD1, VexType::EVEX_512),

    // EVEX immediate versions
    make_evex_instr_reg("vpsrlw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 2, VexType::EVEX_128),
    make_evex_instr_reg("vpsrlw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 2, VexType::EVEX_128),
    make_evex_instr_reg("vpsrlw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 2, VexType::EVEX_256),
    make_evex_instr_reg("vpsrlw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 2, VexType::EVEX_256),
    make_evex_instr_reg("vpsrlw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 2, VexType::EVEX_512),
    make_evex_instr_reg("vpsrlw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 2, VexType::EVEX_512),

    // vpsrld - Packed Shift Right Logical Dword
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsrld", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD2, VexType::VEX_128),
    make_vex_instr("vpsrld", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD2, VexType::VEX_128),
    make_vex_instr("vpsrld", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD2, VexType::VEX_256),
    make_vex_instr("vpsrld", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD2, VexType::VEX_256),

    // VEX immediate versions (reg field = 2)
    make_vex_instr_reg("vpsrld", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 2, VexType::VEX_128),
    make_vex_instr_reg("vpsrld", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 2, VexType::VEX_128),
    make_vex_instr_reg("vpsrld", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 2, VexType::VEX_256),
    make_vex_instr_reg("vpsrld", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 2, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsrld", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD2, VexType::EVEX_128),
    make_evex_instr("vpsrld", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD2, VexType::EVEX_128),
    make_evex_instr("vpsrld", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD2, VexType::EVEX_256),
    make_evex_instr("vpsrld", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD2, VexType::EVEX_256),
    make_evex_instr("vpsrld", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD2, VexType::EVEX_512),
    make_evex_instr("vpsrld", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD2, VexType::EVEX_512),

    // EVEX immediate versions
    make_evex_instr_reg("vpsrld", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 2, VexType::EVEX_128),
    make_evex_instr_reg("vpsrld", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 2, VexType::EVEX_128),
    make_evex_instr_reg("vpsrld", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 2, VexType::EVEX_256),
    make_evex_instr_reg("vpsrld", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 2, VexType::EVEX_256),
    make_evex_instr_reg("vpsrld", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 2, VexType::EVEX_512),
    make_evex_instr_reg("vpsrld", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 2, VexType::EVEX_512),

    // vpsrlq - Packed Shift Right Logical Qword
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsrlq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD3, VexType::VEX_128),
    make_vex_instr("vpsrlq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD3, VexType::VEX_128),
    make_vex_instr("vpsrlq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD3, VexType::VEX_256),
    make_vex_instr("vpsrlq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD3, VexType::VEX_256),

    // VEX immediate versions (reg field = 2)
    make_vex_instr_reg("vpsrlq", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 2, VexType::VEX_128),
    make_vex_instr_reg("vpsrlq", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 2, VexType::VEX_128),
    make_vex_instr_reg("vpsrlq", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 2, VexType::VEX_256),
    make_vex_instr_reg("vpsrlq", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 2, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsrlq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD3, VexType::EVEX_128, true),
    make_evex_instr("vpsrlq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD3, VexType::EVEX_128, true),
    make_evex_instr("vpsrlq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD3, VexType::EVEX_256, true),
    make_evex_instr("vpsrlq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD3, VexType::EVEX_256, true),
    make_evex_instr("vpsrlq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xD3, VexType::EVEX_512, true),
    make_evex_instr("vpsrlq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xD3, VexType::EVEX_512, true),

    // EVEX immediate versions
    make_evex_instr_reg("vpsrlq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 2, VexType::EVEX_128, true),
    make_evex_instr_reg("vpsrlq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 2, VexType::EVEX_128, true),
    make_evex_instr_reg("vpsrlq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 2, VexType::EVEX_256, true),
    make_evex_instr_reg("vpsrlq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 2, VexType::EVEX_256, true),
    make_evex_instr_reg("vpsrlq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 2, VexType::EVEX_512, true),
    make_evex_instr_reg("vpsrlq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 2, VexType::EVEX_512, true),

    // vpsraw - Packed Shift Right Arithmetic Word
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsraw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE1, VexType::VEX_128),
    make_vex_instr("vpsraw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE1, VexType::VEX_128),
    make_vex_instr("vpsraw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE1, VexType::VEX_256),
    make_vex_instr("vpsraw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE1, VexType::VEX_256),

    // VEX immediate versions (reg field = 4)
    make_vex_instr_reg("vpsraw", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 4, VexType::VEX_128),
    make_vex_instr_reg("vpsraw", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 4, VexType::VEX_128),
    make_vex_instr_reg("vpsraw", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 4, VexType::VEX_256),
    make_vex_instr_reg("vpsraw", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 4, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsraw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE1, VexType::EVEX_128),
    make_evex_instr("vpsraw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE1, VexType::EVEX_128),
    make_evex_instr("vpsraw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE1, VexType::EVEX_256),
    make_evex_instr("vpsraw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE1, VexType::EVEX_256),
    make_evex_instr("vpsraw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE1, VexType::EVEX_512),
    make_evex_instr("vpsraw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE1, VexType::EVEX_512),

    // EVEX immediate versions
    make_evex_instr_reg("vpsraw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 4, VexType::EVEX_128),
    make_evex_instr_reg("vpsraw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 4, VexType::EVEX_128),
    make_evex_instr_reg("vpsraw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 4, VexType::EVEX_256),
    make_evex_instr_reg("vpsraw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 4, VexType::EVEX_256),
    make_evex_instr_reg("vpsraw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x71, 4, VexType::EVEX_512),
    make_evex_instr_reg("vpsraw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x71, 4, VexType::EVEX_512),

    // vpsrad - Packed Shift Right Arithmetic Dword
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpsrad", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE2, VexType::VEX_128),
    make_vex_instr("vpsrad", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE2, VexType::VEX_128),
    make_vex_instr("vpsrad", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE2, VexType::VEX_256),
    make_vex_instr("vpsrad", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE2, VexType::VEX_256),

    // VEX immediate versions (reg field = 4)
    make_vex_instr_reg("vpsrad", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::VEX_128),
    make_vex_instr_reg("vpsrad", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::VEX_128),
    make_vex_instr_reg("vpsrad", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::VEX_256),
    make_vex_instr_reg("vpsrad", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsrad", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_128),
    make_evex_instr("vpsrad", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_128),
    make_evex_instr("vpsrad", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_256),
    make_evex_instr("vpsrad", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_256),
    make_evex_instr("vpsrad", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_512),
    make_evex_instr("vpsrad", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_512),

    // EVEX immediate versions
    make_evex_instr_reg("vpsrad", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_128),
    make_evex_instr_reg("vpsrad", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_128),
    make_evex_instr_reg("vpsrad", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_256),
    make_evex_instr_reg("vpsrad", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_256),
    make_evex_instr_reg("vpsrad", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_512),
    make_evex_instr_reg("vpsrad", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_512),

    // vpsraq - Packed Shift Right Arithmetic Qword (AVX-512 only)
    // EVEX versions (AVX-512)
    make_evex_instr("vpsraq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_128, true),
    make_evex_instr("vpsraq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_128, true),
    make_evex_instr("vpsraq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_256, true),
    make_evex_instr("vpsraq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_256, true),
    make_evex_instr("vpsraq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_512, true),
    make_evex_instr("vpsraq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xE2, VexType::EVEX_512, true),

    // EVEX immediate versions (reg field = 4)
    make_evex_instr_reg("vpsraq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_128, true),
    make_evex_instr_reg("vpsraq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_128, true),
    make_evex_instr_reg("vpsraq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_256, true),
    make_evex_instr_reg("vpsraq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_256, true),
    make_evex_instr_reg("vpsraq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_512, true),
    make_evex_instr_reg("vpsraq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x72, 4, VexType::EVEX_512, true),

    // vpslldq - Packed Shift Left Logical Double Quadword (byte granularity)
    // VEX versions (AVX/AVX2) - 2-operand with immediate (reg field = 7)
    make_vex_instr_reg("vpslldq", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 7, VexType::VEX_128),
    make_vex_instr_reg("vpslldq", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 7, VexType::VEX_128),
    make_vex_instr_reg("vpslldq", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 7, VexType::VEX_256),
    make_vex_instr_reg("vpslldq", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 7, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpslldq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 7, VexType::EVEX_128),
    make_evex_instr_reg("vpslldq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 7, VexType::EVEX_128),
    make_evex_instr_reg("vpslldq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 7, VexType::EVEX_256),
    make_evex_instr_reg("vpslldq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 7, VexType::EVEX_256),
    make_evex_instr_reg("vpslldq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 7, VexType::EVEX_512),
    make_evex_instr_reg("vpslldq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 7, VexType::EVEX_512),

    // vpsrldq - Packed Shift Right Logical Double Quadword (byte granularity)
    // VEX versions (AVX/AVX2) - 2-operand with immediate (reg field = 3)
    make_vex_instr_reg("vpsrldq", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 3, VexType::VEX_128),
    make_vex_instr_reg("vpsrldq", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 3, VexType::VEX_128),
    make_vex_instr_reg("vpsrldq", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 3, VexType::VEX_256),
    make_vex_instr_reg("vpsrldq", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 3, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpsrldq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 3, VexType::EVEX_128),
    make_evex_instr_reg("vpsrldq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 3, VexType::EVEX_128),
    make_evex_instr_reg("vpsrldq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 3, VexType::EVEX_256),
    make_evex_instr_reg("vpsrldq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 3, VexType::EVEX_256),
    make_evex_instr_reg("vpsrldq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x73, 3, VexType::EVEX_512),
    make_evex_instr_reg("vpsrldq", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x73, 3, VexType::EVEX_512),

    // vpsllvw - Variable Shift Left Logical Word (AVX-512BW only)
    // EVEX versions (AVX-512BW)
    make_evex_instr("vpsllvw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x12, VexType::EVEX_128),
    make_evex_instr("vpsllvw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x12, VexType::EVEX_128),
    make_evex_instr("vpsllvw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x12, VexType::EVEX_256),
    make_evex_instr("vpsllvw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x12, VexType::EVEX_256),
    make_evex_instr("vpsllvw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x12, VexType::EVEX_512),
    make_evex_instr("vpsllvw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x12, VexType::EVEX_512),

    // vpsllvd - Variable Shift Left Logical Dword
    // VEX versions (AVX2)
    make_vex_instr("vpsllvd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x47, VexType::VEX_128),
    make_vex_instr("vpsllvd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x47, VexType::VEX_128),
    make_vex_instr("vpsllvd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x47, VexType::VEX_256),
    make_vex_instr("vpsllvd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x47, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsllvd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_128),
    make_evex_instr("vpsllvd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_128),
    make_evex_instr("vpsllvd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_256),
    make_evex_instr("vpsllvd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_256),
    make_evex_instr("vpsllvd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_512),
    make_evex_instr("vpsllvd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_512),

    // vpsllvq - Variable Shift Left Logical Qword
    // VEX versions (AVX2)
    make_vex_instr("vpsllvq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x47, VexType::VEX_128, true),
    make_vex_instr("vpsllvq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x47, VexType::VEX_128, true),
    make_vex_instr("vpsllvq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x47, VexType::VEX_256, true),
    make_vex_instr("vpsllvq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x47, VexType::VEX_256, true),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsllvq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_128, true),
    make_evex_instr("vpsllvq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_128, true),
    make_evex_instr("vpsllvq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_256, true),
    make_evex_instr("vpsllvq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_256, true),
    make_evex_instr("vpsllvq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_512, true),
    make_evex_instr("vpsllvq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x47, VexType::EVEX_512, true),

    // vpsrlvw - Variable Shift Right Logical Word (AVX-512BW only)
    // EVEX versions (AVX-512BW)
    make_evex_instr("vpsrlvw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x10, VexType::EVEX_128),
    make_evex_instr("vpsrlvw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x10, VexType::EVEX_128),
    make_evex_instr("vpsrlvw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x10, VexType::EVEX_256),
    make_evex_instr("vpsrlvw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x10, VexType::EVEX_256),
    make_evex_instr("vpsrlvw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x10, VexType::EVEX_512),
    make_evex_instr("vpsrlvw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x10, VexType::EVEX_512),

    // vpsrlvd - Variable Shift Right Logical Dword
    // VEX versions (AVX2)
    make_vex_instr("vpsrlvd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x45, VexType::VEX_128),
    make_vex_instr("vpsrlvd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x45, VexType::VEX_128),
    make_vex_instr("vpsrlvd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x45, VexType::VEX_256),
    make_vex_instr("vpsrlvd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x45, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsrlvd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_128),
    make_evex_instr("vpsrlvd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_128),
    make_evex_instr("vpsrlvd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_256),
    make_evex_instr("vpsrlvd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_256),
    make_evex_instr("vpsrlvd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_512),
    make_evex_instr("vpsrlvd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_512),

    // vpsrlvq - Variable Shift Right Logical Qword
    // VEX versions (AVX2)
    make_vex_instr("vpsrlvq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x45, VexType::VEX_128, true),
    make_vex_instr("vpsrlvq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x45, VexType::VEX_128, true),
    make_vex_instr("vpsrlvq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x45, VexType::VEX_256, true),
    make_vex_instr("vpsrlvq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x45, VexType::VEX_256, true),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsrlvq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_128, true),
    make_evex_instr("vpsrlvq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_128, true),
    make_evex_instr("vpsrlvq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_256, true),
    make_evex_instr("vpsrlvq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_256, true),
    make_evex_instr("vpsrlvq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_512, true),
    make_evex_instr("vpsrlvq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x45, VexType::EVEX_512, true),

    // vpsravw - Variable Shift Right Arithmetic Word (AVX-512BW only)
    // EVEX versions (AVX-512BW)
    make_evex_instr("vpsravw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x11, VexType::EVEX_128),
    make_evex_instr("vpsravw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x11, VexType::EVEX_128),
    make_evex_instr("vpsravw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x11, VexType::EVEX_256),
    make_evex_instr("vpsravw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x11, VexType::EVEX_256),
    make_evex_instr("vpsravw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x11, VexType::EVEX_512),
    make_evex_instr("vpsravw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x11, VexType::EVEX_512),

    // vpsravd - Variable Shift Right Arithmetic Dword
    // VEX versions (AVX2)
    make_vex_instr("vpsravd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x46, VexType::VEX_128),
    make_vex_instr("vpsravd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x46, VexType::VEX_128),
    make_vex_instr("vpsravd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x46, VexType::VEX_256),
    make_vex_instr("vpsravd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x46, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpsravd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_128),
    make_evex_instr("vpsravd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_128),
    make_evex_instr("vpsravd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_256),
    make_evex_instr("vpsravd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_256),
    make_evex_instr("vpsravd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_512),
    make_evex_instr("vpsravd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_512),

    // vpsravq - Variable Shift Right Arithmetic Qword (AVX-512 only)
    // EVEX versions (AVX-512)
    make_evex_instr("vpsravq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_128, true),
    make_evex_instr("vpsravq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_128, true),
    make_evex_instr("vpsravq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_256, true),
    make_evex_instr("vpsravq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_256, true),
    make_evex_instr("vpsravq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_512, true),
    make_evex_instr("vpsravq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x46, VexType::EVEX_512, true),

    // vpshufd - Shuffle Packed Doublewords
    // VEX versions (AVX/AVX2) - 2-operand with immediate
    make_vex_instr_reg("vpshufd", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x70, 0, VexType::VEX_128),
    make_vex_instr_reg("vpshufd", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x70, 0, VexType::VEX_128),
    make_vex_instr_reg("vpshufd", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x70, 0, VexType::VEX_256),
    make_vex_instr_reg("vpshufd", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x70, 0, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpshufd", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x70, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpshufd", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x70, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpshufd", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x70, 0, VexType::EVEX_256),
    make_evex_instr_reg("vpshufd", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x70, 0, VexType::EVEX_256),
    make_evex_instr_reg("vpshufd", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0x70, 0, VexType::EVEX_512),
    make_evex_instr_reg("vpshufd", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x01, 0x70, 0, VexType::EVEX_512),

    // vpshufhw - Shuffle Packed High Words
    // VEX versions (AVX/AVX2) - 2-operand with immediate
    make_vex_instr_reg("vpshufhw", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_F3, 0x01, 0x70, 0, VexType::VEX_128),
    make_vex_instr_reg("vpshufhw", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_F3, 0x01, 0x70, 0, VexType::VEX_128),
    make_vex_instr_reg("vpshufhw", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_F3, 0x01, 0x70, 0, VexType::VEX_256),
    make_vex_instr_reg("vpshufhw", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_F3, 0x01, 0x70, 0, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpshufhw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_F3, 0x01, 0x70, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpshufhw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_F3, 0x01, 0x70, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpshufhw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_F3, 0x01, 0x70, 0, VexType::EVEX_256),
    make_evex_instr_reg("vpshufhw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_F3, 0x01, 0x70, 0, VexType::EVEX_256),
    make_evex_instr_reg("vpshufhw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_F3, 0x01, 0x70, 0, VexType::EVEX_512),
    make_evex_instr_reg("vpshufhw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_F3, 0x01, 0x70, 0, VexType::EVEX_512),

    // vpshuflw - Shuffle Packed Low Words
    // VEX versions (AVX/AVX2) - 2-operand with immediate
    make_vex_instr_reg("vpshuflw", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_F2, 0x01, 0x70, 0, VexType::VEX_128),
    make_vex_instr_reg("vpshuflw", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_F2, 0x01, 0x70, 0, VexType::VEX_128),
    make_vex_instr_reg("vpshuflw", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_F2, 0x01, 0x70, 0, VexType::VEX_256),
    make_vex_instr_reg("vpshuflw", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_F2, 0x01, 0x70, 0, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpshuflw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_F2, 0x01, 0x70, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpshuflw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_F2, 0x01, 0x70, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpshuflw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_F2, 0x01, 0x70, 0, VexType::EVEX_256),
    make_evex_instr_reg("vpshuflw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_F2, 0x01, 0x70, 0, VexType::EVEX_256),
    make_evex_instr_reg("vpshuflw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_F2, 0x01, 0x70, 0, VexType::EVEX_512),
    make_evex_instr_reg("vpshuflw", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_F2, 0x01, 0x70, 0, VexType::EVEX_512),

    // vpshufb - Shuffle Packed Bytes
    // VEX versions (AVX/AVX2) - 3-operand
    make_vex_instr("vpshufb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x00, VexType::VEX_128),
    make_vex_instr("vpshufb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x00, VexType::VEX_128),
    make_vex_instr("vpshufb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x00, VexType::VEX_256),
    make_vex_instr("vpshufb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x00, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpshufb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x00, VexType::EVEX_128),
    make_evex_instr("vpshufb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x00, VexType::EVEX_128),
    make_evex_instr("vpshufb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x00, VexType::EVEX_256),
    make_evex_instr("vpshufb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x00, VexType::EVEX_256),
    make_evex_instr("vpshufb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x00, VexType::EVEX_512),
    make_evex_instr("vpshufb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x00, VexType::EVEX_512),


    // vpshufw - Shuffle Packed Words (MMX-style, rarely used in AVX)
    // Note: This is typically not used in AVX/AVX2 context, but included for completeness

    // vpermilps - Permute In-Lane with Immediate Control
    // VEX versions (AVX/AVX2) - 2-operand with immediate
    make_vex_instr_reg("vpermilps", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x04, 0, VexType::VEX_128),
    make_vex_instr_reg("vpermilps", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x04, 0, VexType::VEX_128),
    make_vex_instr_reg("vpermilps", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x04, 0, VexType::VEX_256),
    make_vex_instr_reg("vpermilps", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x04, 0, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpermilps", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x04, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpermilps", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x04, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpermilps", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x04, 0, VexType::EVEX_256),
    make_evex_instr_reg("vpermilps", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x04, 0, VexType::EVEX_256),
    make_evex_instr_reg("vpermilps", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x04, 0, VexType::EVEX_512),
    make_evex_instr_reg("vpermilps", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x04, 0, VexType::EVEX_512),

    // Variable form (3-operand)
    make_vex_instr("vpermilps", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0C, VexType::VEX_128),
    make_vex_instr("vpermilps", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0C, VexType::VEX_128),
    make_vex_instr("vpermilps", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0C, VexType::VEX_256),
    make_vex_instr("vpermilps", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0C, VexType::VEX_256),

    make_evex_instr("vpermilps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0C, VexType::EVEX_128),
    make_evex_instr("vpermilps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0C, VexType::EVEX_128),
    make_evex_instr("vpermilps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0C, VexType::EVEX_256),
    make_evex_instr("vpermilps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0C, VexType::EVEX_256),
    make_evex_instr("vpermilps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0C, VexType::EVEX_512),
    make_evex_instr("vpermilps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0C, VexType::EVEX_512),

    // vpermilpd - Permute In-Lane with Immediate Control
    // VEX versions (AVX/AVX2) - 2-operand with immediate
    make_vex_instr_reg("vpermilpd", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x05, 0, VexType::VEX_128),
    make_vex_instr_reg("vpermilpd", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x05, 0, VexType::VEX_128),
    make_vex_instr_reg("vpermilpd", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x05, 0, VexType::VEX_256),
    make_vex_instr_reg("vpermilpd", EncodingType::VEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x05, 0, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpermilpd", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x05, 0, VexType::EVEX_128, true),
    make_evex_instr_reg("vpermilpd", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x05, 0, VexType::EVEX_128, true),
    make_evex_instr_reg("vpermilpd", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x05, 0, VexType::EVEX_256, true),
    make_evex_instr_reg("vpermilpd", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x05, 0, VexType::EVEX_256, true),
    make_evex_instr_reg("vpermilpd", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x05, 0, VexType::EVEX_512, true),
    make_evex_instr_reg("vpermilpd", EncodingType::EVEX_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x05, 0, VexType::EVEX_512, true),

    // Variable form (3-operand)
    make_vex_instr("vpermilpd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0D, VexType::VEX_128, true),
    make_vex_instr("vpermilpd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0D, VexType::VEX_128, true),
    make_vex_instr("vpermilpd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0D, VexType::VEX_256, true),
    make_vex_instr("vpermilpd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0D, VexType::VEX_256, true),

    make_evex_instr("vpermilpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0D, VexType::EVEX_128, true),
    make_evex_instr("vpermilpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0D, VexType::EVEX_128, true),
    make_evex_instr("vpermilpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0D, VexType::EVEX_256, true),
    make_evex_instr("vpermilpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0D, VexType::EVEX_256, true),
    make_evex_instr("vpermilpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x0D, VexType::EVEX_512, true),
    make_evex_instr("vpermilpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x0D, VexType::EVEX_512, true),

    // vpblendw - Blend Packed Words
    // VEX versions (AVX/AVX2) - 4-operand with immediate
    make_vex_instr_reg("vpblendw", EncodingType::VEX_REG_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x0E, 0, VexType::VEX_128),
    make_vex_instr_reg("vpblendw", EncodingType::VEX_REG_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x0E, 0, VexType::VEX_128),
    make_vex_instr_reg("vpblendw", EncodingType::VEX_REG_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x0E, 0, VexType::VEX_256),
    make_vex_instr_reg("vpblendw", EncodingType::VEX_REG_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x0E, 0, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpblendw", EncodingType::EVEX_REG_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x0E, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpblendw", EncodingType::EVEX_REG_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x0E, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpblendw", EncodingType::EVEX_REG_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x0E, 0, VexType::EVEX_256),
    make_evex_instr_reg("vpblendw", EncodingType::EVEX_REG_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x0E, 0, VexType::EVEX_256),
    make_evex_instr_reg("vpblendw", EncodingType::EVEX_REG_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x0E, 0, VexType::EVEX_512),
    make_evex_instr_reg("vpblendw", EncodingType::EVEX_REG_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x0E, 0, VexType::EVEX_512),

    // vpblendd - Blend Packed Dwords
    make_vex_instr_reg("vpblendd", EncodingType::VEX_REG_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x02, 0, VexType::VEX_128),
    make_vex_instr_reg("vpblendd", EncodingType::VEX_REG_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x02, 0, VexType::VEX_128),
    make_vex_instr_reg("vpblendd", EncodingType::VEX_REG_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x02, 0, VexType::VEX_256),
    make_vex_instr_reg("vpblendd", EncodingType::VEX_REG_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x02, 0, VexType::VEX_256),

    // vpblendvb - Variable Blend Packed Bytes (4th operand is XMM register)
    make_vex_instr("vpblendvb", EncodingType::VEX_REG_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x4C, VexType::VEX_128),
    make_vex_instr("vpblendvb", EncodingType::VEX_REG_REG_MEM_REG, VEX_PREFIX_66, 0x03, 0x4C, VexType::VEX_128),
    make_vex_instr("vpblendvb", EncodingType::VEX_REG_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x4C, VexType::VEX_256),
    make_vex_instr("vpblendvb", EncodingType::VEX_REG_REG_MEM_REG, VEX_PREFIX_66, 0x03, 0x4C, VexType::VEX_256),

    // vpblendvps - Variable Blend Packed Single-Precision Floating-Point Values
    make_vex_instr("vpblendvps", EncodingType::VEX_REG_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x4A, VexType::VEX_128),
    make_vex_instr("vpblendvps", EncodingType::VEX_REG_REG_MEM_REG, VEX_PREFIX_66, 0x03, 0x4A, VexType::VEX_128),
    make_vex_instr("vpblendvps", EncodingType::VEX_REG_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x4A, VexType::VEX_256),
    make_vex_instr("vpblendvps", EncodingType::VEX_REG_REG_MEM_REG, VEX_PREFIX_66, 0x03, 0x4A, VexType::VEX_256),

    // vpblendvpd - Variable Blend Packed Double-Precision Floating-Point Values
    make_vex_instr("vpblendvpd", EncodingType::VEX_REG_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x4B, VexType::VEX_128),
    make_vex_instr("vpblendvpd", EncodingType::VEX_REG_REG_MEM_REG, VEX_PREFIX_66, 0x03, 0x4B, VexType::VEX_128),
    make_vex_instr("vpblendvpd", EncodingType::VEX_REG_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x4B, VexType::VEX_256),
    make_vex_instr("vpblendvpd", EncodingType::VEX_REG_REG_MEM_REG, VEX_PREFIX_66, 0x03, 0x4B, VexType::VEX_256),

    // vblendps - Blend Packed Single-Precision Floating-Point Values
    make_vex_instr_reg("vblendps", EncodingType::VEX_REG_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x0C, 0, VexType::VEX_128),
    make_vex_instr_reg("vblendps", EncodingType::VEX_REG_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x0C, 0, VexType::VEX_128),
    make_vex_instr_reg("vblendps", EncodingType::VEX_REG_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x0C, 0, VexType::VEX_256),
    make_vex_instr_reg("vblendps", EncodingType::VEX_REG_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x0C, 0, VexType::VEX_256),

    // vblendpd - Blend Packed Double-Precision Floating-Point Values
    make_vex_instr_reg("vblendpd", EncodingType::VEX_REG_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x0D, 0, VexType::VEX_128),
    make_vex_instr_reg("vblendpd", EncodingType::VEX_REG_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x0D, 0, VexType::VEX_128),
    make_vex_instr_reg("vblendpd", EncodingType::VEX_REG_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x0D, 0, VexType::VEX_256),
    make_vex_instr_reg("vblendpd", EncodingType::VEX_REG_REG_MEM_IMM8, VEX_PREFIX_66, 0x03, 0x0D, 0, VexType::VEX_256),


    // vpunpcklbw - Unpack Low Data (Interleave Low Bytes)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpunpcklbw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x60, VexType::VEX_128),
    make_vex_instr("vpunpcklbw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x60, VexType::VEX_128),
    make_vex_instr("vpunpcklbw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x60, VexType::VEX_256),
    make_vex_instr("vpunpcklbw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x60, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpunpcklbw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x60, VexType::EVEX_128),
    make_evex_instr("vpunpcklbw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x60, VexType::EVEX_128),
    make_evex_instr("vpunpcklbw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x60, VexType::EVEX_256),
    make_evex_instr("vpunpcklbw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x60, VexType::EVEX_256),
    make_evex_instr("vpunpcklbw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x60, VexType::EVEX_512),
    make_evex_instr("vpunpcklbw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x60, VexType::EVEX_512),

    // vpunpcklwd - Unpack Low Data (Interleave Low Words)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpunpcklwd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x61, VexType::VEX_128),
    make_vex_instr("vpunpcklwd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x61, VexType::VEX_128),
    make_vex_instr("vpunpcklwd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x61, VexType::VEX_256),
    make_vex_instr("vpunpcklwd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x61, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpunpcklwd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x61, VexType::EVEX_128),
    make_evex_instr("vpunpcklwd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x61, VexType::EVEX_128),
    make_evex_instr("vpunpcklwd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x61, VexType::EVEX_256),
    make_evex_instr("vpunpcklwd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x61, VexType::EVEX_256),
    make_evex_instr("vpunpcklwd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x61, VexType::EVEX_512),
    make_evex_instr("vpunpcklwd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x61, VexType::EVEX_512),

    // vpunpckldq - Unpack Low Data (Interleave Low Dwords)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpunpckldq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x62, VexType::VEX_128),
    make_vex_instr("vpunpckldq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x62, VexType::VEX_128),
    make_vex_instr("vpunpckldq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x62, VexType::VEX_256),
    make_vex_instr("vpunpckldq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x62, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpunpckldq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x62, VexType::EVEX_128),
    make_evex_instr("vpunpckldq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x62, VexType::EVEX_128),
    make_evex_instr("vpunpckldq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x62, VexType::EVEX_256),
    make_evex_instr("vpunpckldq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x62, VexType::EVEX_256),
    make_evex_instr("vpunpckldq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x62, VexType::EVEX_512),
    make_evex_instr("vpunpckldq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x62, VexType::EVEX_512),

    // vpunpcklqdq - Unpack Low Data (Interleave Low Qwords)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpunpcklqdq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6C, VexType::VEX_128),
    make_vex_instr("vpunpcklqdq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6C, VexType::VEX_128),
    make_vex_instr("vpunpcklqdq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6C, VexType::VEX_256),
    make_vex_instr("vpunpcklqdq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6C, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpunpcklqdq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6C, VexType::EVEX_128),
    make_evex_instr("vpunpcklqdq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6C, VexType::EVEX_128),
    make_evex_instr("vpunpcklqdq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6C, VexType::EVEX_256),
    make_evex_instr("vpunpcklqdq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6C, VexType::EVEX_256),
    make_evex_instr("vpunpcklqdq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6C, VexType::EVEX_512),
    make_evex_instr("vpunpcklqdq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6C, VexType::EVEX_512),

    // vpunpckhbw - Unpack High Data (Interleave High Bytes)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpunpckhbw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x68, VexType::VEX_128),
    make_vex_instr("vpunpckhbw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x68, VexType::VEX_128),
    make_vex_instr("vpunpckhbw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x68, VexType::VEX_256),
    make_vex_instr("vpunpckhbw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x68, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpunpckhbw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x68, VexType::EVEX_128),
    make_evex_instr("vpunpckhbw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x68, VexType::EVEX_128),
    make_evex_instr("vpunpckhbw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x68, VexType::EVEX_256),
    make_evex_instr("vpunpckhbw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x68, VexType::EVEX_256),
    make_evex_instr("vpunpckhbw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x68, VexType::EVEX_512),
    make_evex_instr("vpunpckhbw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x68, VexType::EVEX_512),

    // vpunpckhwd - Unpack High Data (Interleave High Words)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpunpckhwd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x69, VexType::VEX_128),
    make_vex_instr("vpunpckhwd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x69, VexType::VEX_128),
    make_vex_instr("vpunpckhwd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x69, VexType::VEX_256),
    make_vex_instr("vpunpckhwd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x69, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpunpckhwd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x69, VexType::EVEX_128),
    make_evex_instr("vpunpckhwd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x69, VexType::EVEX_128),
    make_evex_instr("vpunpckhwd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x69, VexType::EVEX_256),
    make_evex_instr("vpunpckhwd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x69, VexType::EVEX_256),
    make_evex_instr("vpunpckhwd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x69, VexType::EVEX_512),
    make_evex_instr("vpunpckhwd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x69, VexType::EVEX_512),

    // vpunpckhdq - Unpack High Data (Interleave High Dwords)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpunpckhdq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6A, VexType::VEX_128),
    make_vex_instr("vpunpckhdq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6A, VexType::VEX_128),
    make_vex_instr("vpunpckhdq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6A, VexType::VEX_256),
    make_vex_instr("vpunpckhdq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6A, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpunpckhdq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6A, VexType::EVEX_128),
    make_evex_instr("vpunpckhdq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6A, VexType::EVEX_128),
    make_evex_instr("vpunpckhdq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6A, VexType::EVEX_256),
    make_evex_instr("vpunpckhdq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6A, VexType::EVEX_256),
    make_evex_instr("vpunpckhdq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6A, VexType::EVEX_512),
    make_evex_instr("vpunpckhdq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6A, VexType::EVEX_512),

    // vpunpckhqdq - Unpack High Data (Interleave High Qwords)
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpunpckhqdq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6D, VexType::VEX_128),
    make_vex_instr("vpunpckhqdq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6D, VexType::VEX_128),
    make_vex_instr("vpunpckhqdq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6D, VexType::VEX_256),
    make_vex_instr("vpunpckhqdq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6D, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpunpckhqdq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6D, VexType::EVEX_128),
    make_evex_instr("vpunpckhqdq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6D, VexType::EVEX_128),
    make_evex_instr("vpunpckhqdq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6D, VexType::EVEX_256),
    make_evex_instr("vpunpckhqdq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6D, VexType::EVEX_256),
    make_evex_instr("vpunpckhqdq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6D, VexType::EVEX_512),
    make_evex_instr("vpunpckhqdq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6D, VexType::EVEX_512),

    // Floating-point unpack instructions (for completeness)

    // vunpcklps - Unpack and Interleave Low Packed Single-Precision Floating-Point Values
    // VEX versions (AVX/AVX2)
    make_vex_instr("vunpcklps", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x14, VexType::VEX_128),
    make_vex_instr("vunpcklps", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x14, VexType::VEX_128),
    make_vex_instr("vunpcklps", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x14, VexType::VEX_256),
    make_vex_instr("vunpcklps", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x14, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vunpcklps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x14, VexType::EVEX_128),
    make_evex_instr("vunpcklps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x14, VexType::EVEX_128),
    make_evex_instr("vunpcklps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x14, VexType::EVEX_256),
    make_evex_instr("vunpcklps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x14, VexType::EVEX_256),
    make_evex_instr("vunpcklps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x14, VexType::EVEX_512),
    make_evex_instr("vunpcklps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x14, VexType::EVEX_512),

    // vunpcklpd - Unpack and Interleave Low Packed Double-Precision Floating-Point Values
    // VEX versions (AVX/AVX2)
    make_vex_instr("vunpcklpd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x14, VexType::VEX_128),
    make_vex_instr("vunpcklpd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x14, VexType::VEX_128),
    make_vex_instr("vunpcklpd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x14, VexType::VEX_256),
    make_vex_instr("vunpcklpd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x14, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vunpcklpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x14, VexType::EVEX_128, true),
    make_evex_instr("vunpcklpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x14, VexType::EVEX_128, true),
    make_evex_instr("vunpcklpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x14, VexType::EVEX_256, true),
    make_evex_instr("vunpcklpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x14, VexType::EVEX_256, true),
    make_evex_instr("vunpcklpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x14, VexType::EVEX_512, true),
    make_evex_instr("vunpcklpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x14, VexType::EVEX_512, true),

    // vunpckhps - Unpack and Interleave High Packed Single-Precision Floating-Point Values
    // VEX versions (AVX/AVX2)
    make_vex_instr("vunpckhps", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x15, VexType::VEX_128),
    make_vex_instr("vunpckhps", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x15, VexType::VEX_128),
    make_vex_instr("vunpckhps", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x15, VexType::VEX_256),
    make_vex_instr("vunpckhps", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x15, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vunpckhps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x15, VexType::EVEX_128),
    make_evex_instr("vunpckhps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x15, VexType::EVEX_128),
    make_evex_instr("vunpckhps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x15, VexType::EVEX_256),
    make_evex_instr("vunpckhps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x15, VexType::EVEX_256),
    make_evex_instr("vunpckhps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x15, VexType::EVEX_512),
    make_evex_instr("vunpckhps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x15, VexType::EVEX_512),

    // vunpckhpd - Unpack and Interleave High Packed Double-Precision Floating-Point Values
    // VEX versions (AVX/AVX2)
    make_vex_instr("vunpckhpd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x15, VexType::VEX_128),
    make_vex_instr("vunpckhpd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x15, VexType::VEX_128),
    make_vex_instr("vunpckhpd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x15, VexType::VEX_256),
    make_vex_instr("vunpckhpd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x15, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vunpckhpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x15, VexType::EVEX_128, true),
    make_evex_instr("vunpckhpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x15, VexType::EVEX_128, true),
    make_evex_instr("vunpckhpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x15, VexType::EVEX_256, true),
    make_evex_instr("vunpckhpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x15, VexType::EVEX_256, true),
    make_evex_instr("vunpckhpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x15, VexType::EVEX_512, true),
    make_evex_instr("vunpckhpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x15, VexType::EVEX_512, true),

    // vpminsb - Packed Minimum Signed Bytes
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpminsb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x38, VexType::VEX_128),
    make_vex_instr("vpminsb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x38, VexType::VEX_128),
    make_vex_instr("vpminsb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x38, VexType::VEX_256),
    make_vex_instr("vpminsb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x38, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpminsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x38, VexType::EVEX_128),
    make_evex_instr("vpminsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x38, VexType::EVEX_128),
    make_evex_instr("vpminsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x38, VexType::EVEX_256),
    make_evex_instr("vpminsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x38, VexType::EVEX_256),
    make_evex_instr("vpminsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x38, VexType::EVEX_512),
    make_evex_instr("vpminsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x38, VexType::EVEX_512),

    // vpminsw - Packed Minimum Signed Words
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpminsw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEA, VexType::VEX_128),
    make_vex_instr("vpminsw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEA, VexType::VEX_128),
    make_vex_instr("vpminsw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEA, VexType::VEX_256),
    make_vex_instr("vpminsw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEA, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpminsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEA, VexType::EVEX_128),
    make_evex_instr("vpminsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEA, VexType::EVEX_128),
    make_evex_instr("vpminsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEA, VexType::EVEX_256),
    make_evex_instr("vpminsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEA, VexType::EVEX_256),
    make_evex_instr("vpminsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEA, VexType::EVEX_512),
    make_evex_instr("vpminsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEA, VexType::EVEX_512),

    // vpminsd - Packed Minimum Signed Dwords
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpminsd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x39, VexType::VEX_128),
    make_vex_instr("vpminsd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x39, VexType::VEX_128),
    make_vex_instr("vpminsd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x39, VexType::VEX_256),
    make_vex_instr("vpminsd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x39, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpminsd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_128),
    make_evex_instr("vpminsd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_128),
    make_evex_instr("vpminsd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_256),
    make_evex_instr("vpminsd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_256),
    make_evex_instr("vpminsd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_512),
    make_evex_instr("vpminsd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_512),

    // vpminsq - Packed Minimum Signed Qwords (AVX-512 only)
    // EVEX versions (AVX-512)
    make_evex_instr("vpminsq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_128, true),
    make_evex_instr("vpminsq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_128, true),
    make_evex_instr("vpminsq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_256, true),
    make_evex_instr("vpminsq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_256, true),
    make_evex_instr("vpminsq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_512, true),
    make_evex_instr("vpminsq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x39, VexType::EVEX_512, true),

    // vpminub - Packed Minimum Unsigned Bytes
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpminub", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDA, VexType::VEX_128),
    make_vex_instr("vpminub", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDA, VexType::VEX_128),
    make_vex_instr("vpminub", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDA, VexType::VEX_256),
    make_vex_instr("vpminub", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDA, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpminub", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDA, VexType::EVEX_128),
    make_evex_instr("vpminub", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDA, VexType::EVEX_128),
    make_evex_instr("vpminub", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDA, VexType::EVEX_256),
    make_evex_instr("vpminub", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDA, VexType::EVEX_256),
    make_evex_instr("vpminub", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDA, VexType::EVEX_512),
    make_evex_instr("vpminub", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDA, VexType::EVEX_512),

    // vpminuw - Packed Minimum Unsigned Words
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpminuw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3A, VexType::VEX_128),
    make_vex_instr("vpminuw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3A, VexType::VEX_128),
    make_vex_instr("vpminuw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3A, VexType::VEX_256),
    make_vex_instr("vpminuw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3A, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpminuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3A, VexType::EVEX_128),
    make_evex_instr("vpminuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3A, VexType::EVEX_128),
    make_evex_instr("vpminuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3A, VexType::EVEX_256),
    make_evex_instr("vpminuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3A, VexType::EVEX_256),
    make_evex_instr("vpminuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3A, VexType::EVEX_512),
    make_evex_instr("vpminuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3A, VexType::EVEX_512),

    // vpminud - Packed Minimum Unsigned Dwords
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpminud", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3B, VexType::VEX_128),
    make_vex_instr("vpminud", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3B, VexType::VEX_128),
    make_vex_instr("vpminud", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3B, VexType::VEX_256),
    make_vex_instr("vpminud", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3B, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpminud", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_128),
    make_evex_instr("vpminud", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_128),
    make_evex_instr("vpminud", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_256),
    make_evex_instr("vpminud", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_256),
    make_evex_instr("vpminud", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_512),
    make_evex_instr("vpminud", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_512),

    // vpminuq - Packed Minimum Unsigned Qwords (AVX-512 only)
    // EVEX versions (AVX-512)
    make_evex_instr("vpminuq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_128, true),
    make_evex_instr("vpminuq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_128, true),
    make_evex_instr("vpminuq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_256, true),
    make_evex_instr("vpminuq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_256, true),
    make_evex_instr("vpminuq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_512, true),
    make_evex_instr("vpminuq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3B, VexType::EVEX_512, true),

    // vpmaxsb - Packed Maximum Signed Bytes
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmaxsb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3C, VexType::VEX_128),
    make_vex_instr("vpmaxsb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3C, VexType::VEX_128),
    make_vex_instr("vpmaxsb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3C, VexType::VEX_256),
    make_vex_instr("vpmaxsb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3C, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmaxsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3C, VexType::EVEX_128),
    make_evex_instr("vpmaxsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3C, VexType::EVEX_128),
    make_evex_instr("vpmaxsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3C, VexType::EVEX_256),
    make_evex_instr("vpmaxsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3C, VexType::EVEX_256),
    make_evex_instr("vpmaxsb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3C, VexType::EVEX_512),
    make_evex_instr("vpmaxsb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3C, VexType::EVEX_512),

    // vpmaxsw - Packed Maximum Signed Words
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmaxsw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEE, VexType::VEX_128),
    make_vex_instr("vpmaxsw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEE, VexType::VEX_128),
    make_vex_instr("vpmaxsw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEE, VexType::VEX_256),
    make_vex_instr("vpmaxsw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEE, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmaxsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEE, VexType::EVEX_128),
    make_evex_instr("vpmaxsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEE, VexType::EVEX_128),
    make_evex_instr("vpmaxsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEE, VexType::EVEX_256),
    make_evex_instr("vpmaxsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEE, VexType::EVEX_256),
    make_evex_instr("vpmaxsw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xEE, VexType::EVEX_512),
    make_evex_instr("vpmaxsw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xEE, VexType::EVEX_512),

    // vpmaxsd - Packed Maximum Signed Dwords
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmaxsd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3D, VexType::VEX_128),
    make_vex_instr("vpmaxsd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3D, VexType::VEX_128),
    make_vex_instr("vpmaxsd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3D, VexType::VEX_256),
    make_vex_instr("vpmaxsd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3D, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmaxsd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_128),
    make_evex_instr("vpmaxsd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_128),
    make_evex_instr("vpmaxsd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_256),
    make_evex_instr("vpmaxsd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_256),
    make_evex_instr("vpmaxsd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_512),
    make_evex_instr("vpmaxsd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_512),

    // vpmaxsq - Packed Maximum Signed Qwords (AVX-512 only)
    // EVEX versions (AVX-512)
    make_evex_instr("vpmaxsq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_128, true),
    make_evex_instr("vpmaxsq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_128, true),
    make_evex_instr("vpmaxsq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_256, true),
    make_evex_instr("vpmaxsq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_256, true),
    make_evex_instr("vpmaxsq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_512, true),
    make_evex_instr("vpmaxsq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3D, VexType::EVEX_512, true),

    // vpmaxub - Packed Maximum Unsigned Bytes
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmaxub", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDE, VexType::VEX_128),
    make_vex_instr("vpmaxub", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDE, VexType::VEX_128),
    make_vex_instr("vpmaxub", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDE, VexType::VEX_256),
    make_vex_instr("vpmaxub", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDE, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmaxub", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDE, VexType::EVEX_128),
    make_evex_instr("vpmaxub", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDE, VexType::EVEX_128),
    make_evex_instr("vpmaxub", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDE, VexType::EVEX_256),
    make_evex_instr("vpmaxub", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDE, VexType::EVEX_256),
    make_evex_instr("vpmaxub", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xDE, VexType::EVEX_512),
    make_evex_instr("vpmaxub", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xDE, VexType::EVEX_512),

    // vpmaxuw - Packed Maximum Unsigned Words
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmaxuw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3E, VexType::VEX_128),
    make_vex_instr("vpmaxuw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3E, VexType::VEX_128),
    make_vex_instr("vpmaxuw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3E, VexType::VEX_256),
    make_vex_instr("vpmaxuw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3E, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpmaxuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3E, VexType::EVEX_128),
    make_evex_instr("vpmaxuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3E, VexType::EVEX_128),
    make_evex_instr("vpmaxuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3E, VexType::EVEX_256),
    make_evex_instr("vpmaxuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3E, VexType::EVEX_256),
    make_evex_instr("vpmaxuw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3E, VexType::EVEX_512),
    make_evex_instr("vpmaxuw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3E, VexType::EVEX_512),

    // vpmaxud - Packed Maximum Unsigned Dwords
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpmaxud", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3F, VexType::VEX_128),
    make_vex_instr("vpmaxud", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3F, VexType::VEX_128),
    make_vex_instr("vpmaxud", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3F, VexType::VEX_256),
    make_vex_instr("vpmaxud", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3F, VexType::VEX_256),
    // EVEX versions (AVX-512)
    make_evex_instr("vpmaxud", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_128),
    make_evex_instr("vpmaxud", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_128),
    make_evex_instr("vpmaxud", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_256),
    make_evex_instr("vpmaxud", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_256),
    make_evex_instr("vpmaxud", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_512),
    make_evex_instr("vpmaxud", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_512),
    // vpmaxuq - Packed Maximum Unsigned Qwords (AVX-512 only)
    // EVEX versions (AVX-512)
    make_evex_instr("vpmaxuq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_128, true),
    make_evex_instr("vpmaxuq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_128, true),
    make_evex_instr("vpmaxuq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_256, true),
    make_evex_instr("vpmaxuq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_256, true),
    make_evex_instr("vpmaxuq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_512, true),
    make_evex_instr("vpmaxuq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x3F, VexType::EVEX_512, true),
    // Floating-point min/max instructions (for completeness)
    // vminps - Minimum Packed Single-Precision Floating-Point Values
    // VEX versions (AVX/AVX2)
    make_vex_instr("vminps", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x5D, VexType::VEX_128),
    make_vex_instr("vminps", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x5D, VexType::VEX_128),
    make_vex_instr("vminps", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x5D, VexType::VEX_256),
    make_vex_instr("vminps", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x5D, VexType::VEX_256),
    // EVEX versions (AVX-512)
    make_evex_instr("vminps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x5D, VexType::EVEX_128),
    make_evex_instr("vminps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x5D, VexType::EVEX_128),
    make_evex_instr("vminps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x5D, VexType::EVEX_256),
    make_evex_instr("vminps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x5D, VexType::EVEX_256),
    make_evex_instr("vminps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x5D, VexType::EVEX_512),
    make_evex_instr("vminps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x5D, VexType::EVEX_512),
    // vminpd - Minimum Packed Double-Precision Floating-Point Values
    // VEX versions (AVX/AVX2)
    make_vex_instr("vminpd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x5D, VexType::VEX_128),
    make_vex_instr("vminpd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x5D, VexType::VEX_128),
    make_vex_instr("vminpd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x5D, VexType::VEX_256),
    make_vex_instr("vminpd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x5D, VexType::VEX_256),
    // EVEX versions (AVX-512)
    make_evex_instr("vminpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x5D, VexType::EVEX_128, true),
    make_evex_instr("vminpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x5D, VexType::EVEX_128, true),
    make_evex_instr("vminpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x5D, VexType::EVEX_256, true),
    make_evex_instr("vminpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x5D, VexType::EVEX_256, true),
    make_evex_instr("vminpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x5D, VexType::EVEX_512, true),
    make_evex_instr("vminpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x5D, VexType::EVEX_512, true),
    // vmaxps - Maximum Packed Single-Precision Floating-Point Values
    // VEX versions (AVX/AVX2)
    make_vex_instr("vmaxps", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x5F, VexType::VEX_128),
    make_vex_instr("vmaxps", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x5F, VexType::VEX_128),
    make_vex_instr("vmaxps", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x5F, VexType::VEX_256),
    make_vex_instr("vmaxps", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x5F, VexType::VEX_256),
    // EVEX versions (AVX-512)
    make_evex_instr("vmaxps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x5F, VexType::EVEX_128),
    make_evex_instr("vmaxps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x5F, VexType::EVEX_128),
    make_evex_instr("vmaxps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x5F, VexType::EVEX_256),
    make_evex_instr("vmaxps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x5F, VexType::EVEX_256),
    make_evex_instr("vmaxps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_NONE, 0x01, 0x5F, VexType::EVEX_512),
    make_evex_instr("vmaxps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_NONE, 0x01, 0x5F, VexType::EVEX_512),
    // vmaxpd - Maximum Packed Double-Precision Floating-Point Values
    // VEX versions (AVX/AVX2)
    make_vex_instr("vmaxpd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x5F, VexType::VEX_128),
    make_vex_instr("vmaxpd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x5F, VexType::VEX_128),
    make_vex_instr("vmaxpd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x5F, VexType::VEX_256),
    make_vex_instr("vmaxpd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x5F, VexType::VEX_256),
    // EVEX versions (AVX-512)
    make_evex_instr("vmaxpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x5F, VexType::EVEX_128, true),
    make_evex_instr("vmaxpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x5F, VexType::EVEX_128, true),
    make_evex_instr("vmaxpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x5F, VexType::EVEX_256, true),
    make_evex_instr("vmaxpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x5F, VexType::EVEX_256, true),
    make_evex_instr("vmaxpd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x5F, VexType::EVEX_512, true),
    make_evex_instr("vmaxpd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x5F, VexType::EVEX_512, true),

    // vpacksswb - Pack Signed Words to Signed Bytes with Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpacksswb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x63, VexType::VEX_128),
    make_vex_instr("vpacksswb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x63, VexType::VEX_128),
    make_vex_instr("vpacksswb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x63, VexType::VEX_256),
    make_vex_instr("vpacksswb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x63, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpacksswb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x63, VexType::EVEX_128),
    make_evex_instr("vpacksswb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x63, VexType::EVEX_128),
    make_evex_instr("vpacksswb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x63, VexType::EVEX_256),
    make_evex_instr("vpacksswb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x63, VexType::EVEX_256),
    make_evex_instr("vpacksswb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x63, VexType::EVEX_512),
    make_evex_instr("vpacksswb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x63, VexType::EVEX_512),

    // vpackssdw - Pack Signed Dwords to Signed Words with Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpackssdw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6B, VexType::VEX_128),
    make_vex_instr("vpackssdw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6B, VexType::VEX_128),
    make_vex_instr("vpackssdw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6B, VexType::VEX_256),
    make_vex_instr("vpackssdw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6B, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpackssdw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6B, VexType::EVEX_128),
    make_evex_instr("vpackssdw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6B, VexType::EVEX_128),
    make_evex_instr("vpackssdw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6B, VexType::EVEX_256),
    make_evex_instr("vpackssdw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6B, VexType::EVEX_256),
    make_evex_instr("vpackssdw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x6B, VexType::EVEX_512),
    make_evex_instr("vpackssdw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x6B, VexType::EVEX_512),

    // vpackuswb - Pack Unsigned Words to Unsigned Bytes with Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpackuswb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x67, VexType::VEX_128),
    make_vex_instr("vpackuswb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x67, VexType::VEX_128),
    make_vex_instr("vpackuswb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x67, VexType::VEX_256),
    make_vex_instr("vpackuswb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x67, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpackuswb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x67, VexType::EVEX_128),
    make_evex_instr("vpackuswb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x67, VexType::EVEX_128),
    make_evex_instr("vpackuswb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x67, VexType::EVEX_256),
    make_evex_instr("vpackuswb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x67, VexType::EVEX_256),
    make_evex_instr("vpackuswb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0x67, VexType::EVEX_512),
    make_evex_instr("vpackuswb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0x67, VexType::EVEX_512),

    // vpackusdw - Pack Unsigned Dwords to Unsigned Words with Saturation
    // VEX versions (AVX/AVX2)
    make_vex_instr("vpackusdw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x2B, VexType::VEX_128),
    make_vex_instr("vpackusdw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x2B, VexType::VEX_128),
    make_vex_instr("vpackusdw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x2B, VexType::VEX_256),
    make_vex_instr("vpackusdw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x2B, VexType::VEX_256),

    // EVEX versions (AVX-512)
    make_evex_instr("vpackusdw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x2B, VexType::EVEX_128),
    make_evex_instr("vpackusdw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x2B, VexType::EVEX_128),
    make_evex_instr("vpackusdw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x2B, VexType::EVEX_256),
    make_evex_instr("vpackusdw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x2B, VexType::EVEX_256),
    make_evex_instr("vpackusdw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x02, 0x2B, VexType::EVEX_512),
    make_evex_instr("vpackusdw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x02, 0x2B, VexType::EVEX_512),


    // vpinsrb - Insert Byte
    // VEX versions (AVX/AVX2) - 3-operand with immediate
    make_vex_instr_reg("vpinsrb", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x20, 0, VexType::VEX_128),
    make_vex_instr_reg("vpinsrb", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x20, 0, VexType::VEX_128),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpinsrb", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x20, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpinsrb", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x20, 0, VexType::EVEX_128),

    // vpinsrw - Insert Word
    // VEX versions (AVX/AVX2) - 3-operand with immediate
    make_vex_instr_reg("vpinsrw", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xC4, 0, VexType::VEX_128),
    make_vex_instr_reg("vpinsrw", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xC4, 0, VexType::VEX_128),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpinsrw", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x01, 0xC4, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpinsrw", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x01, 0xC4, 0, VexType::EVEX_128),

    // vpinsrd - Insert Dword
    // VEX versions (AVX/AVX2) - 3-operand with immediate
    make_vex_instr_reg("vpinsrd", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x22, 0, VexType::VEX_128),
    make_vex_instr_reg("vpinsrd", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x22, 0, VexType::VEX_128),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpinsrd", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x22, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpinsrd", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x22, 0, VexType::EVEX_128),

    // vpinsrq - Insert Qword
    // VEX versions (AVX/AVX2) - 3-operand with immediate (requires REX.W)
    make_vex_instr_reg("vpinsrq", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x22, 0, VexType::VEX_128, true),
    make_vex_instr_reg("vpinsrq", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x22, 0, VexType::VEX_128, true),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpinsrq", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x22, 0, VexType::EVEX_128, true),
    make_evex_instr_reg("vpinsrq", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x22, 0, VexType::EVEX_128, true),

    // vpextrb - Extract Byte
    // VEX versions (AVX/AVX2) - 2-operand with immediate (extract to register or memory)
    make_vex_instr_reg("vpextrb", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x14, 0, VexType::VEX_128),
    make_vex_instr_reg("vpextrb", EncodingType::VEX_MEM_REG_IMM8, VEX_PREFIX_66, 0x03, 0x14, 0, VexType::VEX_128),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpextrb", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x14, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpextrb", EncodingType::EVEX_MEM_REG_IMM8, VEX_PREFIX_66, 0x03, 0x14, 0, VexType::EVEX_128),

    // vpextrw - Extract Word
    // VEX versions (AVX/AVX2) - 2-operand with immediate
    make_vex_instr_reg("vpextrw", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0xC5, 0, VexType::VEX_128),
    // Note: vpextrw also has a 3-byte opcode form for memory destination
    make_vex_instr_reg("vpextrw", EncodingType::VEX_MEM_REG_IMM8, VEX_PREFIX_66, 0x03, 0x15, 0, VexType::VEX_128),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpextrw", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x01, 0xC5, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpextrw", EncodingType::EVEX_MEM_REG_IMM8, VEX_PREFIX_66, 0x03, 0x15, 0, VexType::EVEX_128),

    // vpextrd - Extract Dword
    // VEX versions (AVX/AVX2) - 2-operand with immediate
    make_vex_instr_reg("vpextrd", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x16, 0, VexType::VEX_128),
    make_vex_instr_reg("vpextrd", EncodingType::VEX_MEM_REG_IMM8, VEX_PREFIX_66, 0x03, 0x16, 0, VexType::VEX_128),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpextrd", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x16, 0, VexType::EVEX_128),
    make_evex_instr_reg("vpextrd", EncodingType::EVEX_MEM_REG_IMM8, VEX_PREFIX_66, 0x03, 0x16, 0, VexType::EVEX_128),

    // vpextrq - Extract Qword
    // VEX versions (AVX/AVX2) - 2-operand with immediate (requires REX.W)
    make_vex_instr_reg("vpextrq", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x16, 0, VexType::VEX_128, true),
    make_vex_instr_reg("vpextrq", EncodingType::VEX_MEM_REG_IMM8, VEX_PREFIX_66, 0x03, 0x16, 0, VexType::VEX_128, true),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vpextrq", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x16, 0, VexType::EVEX_128, true),
    make_evex_instr_reg("vpextrq", EncodingType::EVEX_MEM_REG_IMM8, VEX_PREFIX_66, 0x03, 0x16, 0, VexType::EVEX_128, true),

    // vinsertps - Insert Packed Single Precision Floating-Point Value
    // VEX versions (AVX/AVX2) - 3-operand with immediate
    make_vex_instr_reg("vinsertps", EncodingType::VEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x21, 0, VexType::VEX_128),
    make_vex_instr_reg("vinsertps", EncodingType::VEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x21, 0, VexType::VEX_128),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vinsertps", EncodingType::EVEX_REG_REG_REG, VEX_PREFIX_66, 0x03, 0x21, 0, VexType::EVEX_128),
    make_evex_instr_reg("vinsertps", EncodingType::EVEX_REG_REG_MEM, VEX_PREFIX_66, 0x03, 0x21, 0, VexType::EVEX_128),

    // vextractps - Extract Packed Single Precision Floating-Point Value
    // VEX versions (AVX/AVX2) - 2-operand with immediate
    make_vex_instr_reg("vextractps", EncodingType::VEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x17, 0, VexType::VEX_128),
    make_vex_instr_reg("vextractps", EncodingType::VEX_MEM_REG_IMM8, VEX_PREFIX_66, 0x03, 0x17, 0, VexType::VEX_128),

    // EVEX versions (AVX-512)
    make_evex_instr_reg("vextractps", EncodingType::EVEX_REG_REG_IMM8, VEX_PREFIX_66, 0x03, 0x17, 0, VexType::EVEX_128),
    make_evex_instr_reg("vextractps", EncodingType::EVEX_MEM_REG_IMM8, VEX_PREFIX_66, 0x03, 0x17, 0, VexType::EVEX_128),

    // pinsrb/pextrb also need legacy SSE4.1 forms (non-VEX) for completeness
    // But since you only want AVX/AVX2/AVX-512, we'll skip those

    make_vex_instr("vaddps", EncodingType::VEX_REG_REG_REG, 0x00, 0x01, 0x58, VexType::VEX_128),
    make_vex_instr("vaddps", EncodingType::VEX_REG_REG_MEM, 0x00, 0x01, 0x58, VexType::VEX_128),
    make_vex_instr("vaddps", EncodingType::VEX_REG_REG_REG, 0x00, 0x01, 0x58, VexType::VEX_256),
    make_vex_instr("vaddps", EncodingType::VEX_REG_REG_MEM, 0x00, 0x01, 0x58, VexType::VEX_256),
    make_vex_instr("vmulps", EncodingType::VEX_REG_REG_REG, 0x00, 0x01, 0x59, VexType::VEX_128),
    make_vex_instr("vmulps", EncodingType::VEX_REG_REG_MEM, 0x00, 0x01, 0x59, VexType::VEX_128),
    make_vex_instr("vmulps", EncodingType::VEX_REG_REG_REG, 0x00, 0x01, 0x59, VexType::VEX_256),
    make_vex_instr("vsubps", EncodingType::VEX_REG_REG_REG, 0x00, 0x01, 0x5C, VexType::VEX_128),
    make_vex_instr("vsubps", EncodingType::VEX_REG_REG_REG, 0x00, 0x01, 0x5C, VexType::VEX_256),
    make_evex_instr("vaddps", EncodingType::EVEX_REG_REG_REG, 0x00, 0x01, 0x58, VexType::EVEX_128),
    make_evex_instr("vaddps", EncodingType::EVEX_REG_REG_MEM, 0x00, 0x01, 0x58, VexType::EVEX_128),
    make_evex_instr("vmulps", EncodingType::EVEX_REG_REG_REG, 0x00, 0x01, 0x59, VexType::EVEX_128),
    make_evex_instr("vmulps", EncodingType::EVEX_REG_REG_MEM, 0x00, 0x01, 0x59, VexType::EVEX_128),
    make_evex_instr("vsubps", EncodingType::EVEX_REG_REG_REG, 0x00, 0x01, 0x5C, VexType::EVEX_128),
    make_evex_instr("vsubps", EncodingType::EVEX_REG_REG_MEM, 0x00, 0x01, 0x5C, VexType::EVEX_128),
    make_evex_instr("vfmadd213ps", EncodingType::EVEX_REG_REG_REG, 0x02, 0x02, 0xA8, VexType::EVEX_128),
    make_evex_instr("vfmadd213ps", EncodingType::EVEX_REG_REG_MEM, 0x02, 0x02, 0xA8, VexType::EVEX_128),
    make_evex_instr("vbroadcastss", EncodingType::EVEX_REG_REG_MEM, 0x02, 0x02, 0x18, VexType::EVEX_128),
    make_evex_instr("vaddps", EncodingType::EVEX_REG_REG_REG, 0x00, 0x01, 0x58, VexType::EVEX_256),
    make_evex_instr("vaddps", EncodingType::EVEX_REG_REG_MEM, 0x00, 0x01, 0x58, VexType::EVEX_256),
    make_evex_instr("vmulps", EncodingType::EVEX_REG_REG_REG, 0x00, 0x01, 0x59, VexType::EVEX_256),
    make_evex_instr("vmulps", EncodingType::EVEX_REG_REG_MEM, 0x00, 0x01, 0x59, VexType::EVEX_256),
    make_evex_instr("vsubps", EncodingType::EVEX_REG_REG_REG, 0x00, 0x01, 0x5C, VexType::EVEX_256),
    make_evex_instr("vsubps", EncodingType::EVEX_REG_REG_MEM, 0x00, 0x01, 0x5C, VexType::EVEX_256),
    make_evex_instr("vfmadd213ps", EncodingType::EVEX_REG_REG_REG, 0x02, 0x02, 0xA8, VexType::EVEX_256),
    make_evex_instr("vfmadd213ps", EncodingType::EVEX_REG_REG_MEM, 0x02, 0x02, 0xA8, VexType::EVEX_256),
    make_evex_instr("vbroadcastss", EncodingType::EVEX_REG_REG_MEM, 0x02, 0x02, 0x18, VexType::EVEX_256),
    make_evex_instr("vaddps", EncodingType::EVEX_REG_REG_REG, 0x00, 0x01, 0x58, VexType::EVEX_512),
    make_evex_instr("vaddps", EncodingType::EVEX_REG_REG_MEM, 0x00, 0x01, 0x58, VexType::EVEX_512),
    make_evex_instr("vmulps", EncodingType::EVEX_REG_REG_REG, 0x00, 0x01, 0x59, VexType::EVEX_512),
    make_evex_instr("vmulps", EncodingType::EVEX_REG_REG_MEM, 0x00, 0x01, 0x59, VexType::EVEX_512),
    make_evex_instr("vsubps", EncodingType::EVEX_REG_REG_REG, 0x00, 0x01, 0x5C, VexType::EVEX_512),
    make_evex_instr("vsubps", EncodingType::EVEX_REG_REG_MEM, 0x00, 0x01, 0x5C, VexType::EVEX_512),
    make_evex_instr("vfmadd213ps", EncodingType::EVEX_REG_REG_REG, 0x02, 0x02, 0xA8, VexType::EVEX_512),
    make_evex_instr("vfmadd213ps", EncodingType::EVEX_REG_REG_MEM, 0x02, 0x02, 0xA8, VexType::EVEX_512),
    make_evex_instr("vbroadcastss", EncodingType::EVEX_REG_REG_MEM, 0x02, 0x02, 0x18, VexType::EVEX_512),


    { "debug", EncodingType::NO_OPERANDS, 0, 0xCC, 0, 0, 0, false, false, false, false, false, VexType::NONE, 0, false },

    // *** TODO: needs support for 32 operands + immediate ***
    // vshufps - Shuffle Packed Single-Precision Floating-Point Values
    // VEX versions (AVX/AVX2) - 3-operand with immediate
    // EVEX versions (AVX-512)
    // vshufpd - Shuffle Packed Double-Precision Floating-Point Values
    // VEX versions (AVX/AVX2) - 3-operand with immediate
    // EVEX versions (AVX-512)

}};

constexpr EncodingType get_encoding_type(const Operand* ops, size_t count) {
    if (count == 0) return EncodingType::NO_OPERANDS;
    if (count == 1) {
        if (ops[0].type == OperandType::JumpLabel) return EncodingType::JUMP_LABEL;
        if (ops[0].type == OperandType::LongJumpLabel) return EncodingType::LONG_JUMP_LABEL;
        if (ops[0].type == OperandType::Register || ops[0].type == OperandType::RegisterPlaceholder) return EncodingType::SINGLE_REG;
        if (ops[0].type == OperandType::Memory || ops[0].type == OperandType::PointerPlaceholder) return EncodingType::SINGLE_MEM;
        return EncodingType::NO_OPERANDS;
    }
    if (count == 2) {
        auto dst = ops[0].type, src = ops[1].type;
        if (dst == OperandType::PointerPlaceholder) dst = OperandType::Memory;
        if (src == OperandType::PointerPlaceholder) src = OperandType::Memory;
        if (dst == OperandType::RegisterPlaceholder) dst = OperandType::Register;
        if (src == OperandType::RegisterPlaceholder) src = OperandType::Register;
        if (dst == OperandType::ImmediatePlaceholder) dst = OperandType::Immediate;
        if (src == OperandType::ImmediatePlaceholder) src = OperandType::Immediate;

        if (dst == OperandType::Register && src == OperandType::Register) return EncodingType::REG_REG;
        if (dst == OperandType::Register && (src == OperandType::Immediate || src == OperandType::ImmediatePlaceholder)) {
            return fits_in_imm8(ops[1].immediate) ? EncodingType::REG_IMM8 : EncodingType::REG_IMM;
        }
        if (dst == OperandType::Register && src == OperandType::Memory) return EncodingType::REG_MEM;
        if (dst == OperandType::Memory && src == OperandType::Register) return EncodingType::MEM_REG;
        if (dst == OperandType::Memory && (src == OperandType::Immediate || src == OperandType::ImmediatePlaceholder)) {
            return fits_in_imm8(ops[1].immediate) ? EncodingType::MEM_IMM8 : EncodingType::MEM_IMM;
        }
    }
    if (count == 3) {
        auto dst = ops[0].type, src1 = ops[1].type, src2 = ops[2].type;
        if (dst == OperandType::PointerPlaceholder) dst = OperandType::Memory;
        if (src1 == OperandType::PointerPlaceholder) src1 = OperandType::Memory;
        if (src2 == OperandType::PointerPlaceholder) src2 = OperandType::Memory;
        if (dst == OperandType::RegisterPlaceholder) dst = OperandType::Register;
        if (src1 == OperandType::RegisterPlaceholder) src1 = OperandType::Register;
        if (src2 == OperandType::RegisterPlaceholder) src2 = OperandType::Register;
        if (dst == OperandType::ImmediatePlaceholder) dst = OperandType::Immediate;    // ADD THIS
        if (src1 == OperandType::ImmediatePlaceholder) src1 = OperandType::Immediate;  // ADD THIS
        if (src2 == OperandType::ImmediatePlaceholder) src2 = OperandType::Immediate;  // ADD THIS

        // ADD THIS: Check for MEM, REG, IMM pattern (for pextr instructions)
        if (dst == OperandType::Memory && src1 == OperandType::Register &&
            (src2 == OperandType::Immediate || src2 == OperandType::ImmediatePlaceholder)) {
            bool has_mask = (ops[0].opmask != Reg::NONE);
            return has_mask ? EncodingType::EVEX_MEM_REG_IMM8 : EncodingType::VEX_MEM_REG_IMM8;
        }

        // Check for three-operand IMUL: dst=reg, src1=reg/mem, src2=imm
        if (dst == OperandType::Register &&
            (src2 == OperandType::Immediate || src2 == OperandType::ImmediatePlaceholder)) {
            bool is_imm8 = fits_in_imm8(ops[2].immediate);
            if (src1 == OperandType::Register) {
                return is_imm8 ? EncodingType::REG_REG_IMM8 : EncodingType::REG_REG_IMM;
            }
            if (src1 == OperandType::Memory) {
                return is_imm8 ? EncodingType::REG_MEM_IMM8 : EncodingType::REG_MEM_IMM;
            }
        }

        bool has_mask = (ops[0].opmask != Reg::NONE);

        // Check if this is a VEX/EVEX instruction with immediate
        if (dst == OperandType::Register && src1 == OperandType::Register &&
            (src2 == OperandType::Immediate || src2 == OperandType::ImmediatePlaceholder)) {
            // This is a VEX/EVEX REG, REG, IMM8 form
            return has_mask ? EncodingType::EVEX_REG_REG_IMM8 : EncodingType::VEX_REG_REG_IMM8;
        }
        if (dst == OperandType::Register && src1 == OperandType::Memory &&
            (src2 == OperandType::Immediate || src2 == OperandType::ImmediatePlaceholder)) {
            // This is a VEX/EVEX REG, MEM, IMM8 form
            return has_mask ? EncodingType::EVEX_REG_MEM_IMM8 : EncodingType::VEX_REG_MEM_IMM8;
        }
        if (dst == OperandType::Register && src1 == OperandType::Register && src2 == OperandType::Register) {
            return has_mask ? EncodingType::EVEX_REG_REG_REG : EncodingType::VEX_REG_REG_REG;
        }
        if (dst == OperandType::Register && src1 == OperandType::Register && src2 == OperandType::Memory) {
            return has_mask ? EncodingType::EVEX_REG_REG_MEM : EncodingType::VEX_REG_REG_MEM;
        }
    }
    if (count == 4) {
        auto dst = ops[0].type, src1 = ops[1].type, src2 = ops[2].type, src3 = ops[3].type;

        // Normalize placeholder types
        if (dst == OperandType::RegisterPlaceholder) dst = OperandType::Register;
        if (src1 == OperandType::RegisterPlaceholder) src1 = OperandType::Register;
        if (src2 == OperandType::RegisterPlaceholder) src2 = OperandType::Register;
        if (src3 == OperandType::RegisterPlaceholder) src3 = OperandType::Register;
        if (src2 == OperandType::PointerPlaceholder) src2 = OperandType::Memory;
        if (src3 == OperandType::ImmediatePlaceholder) src3 = OperandType::Immediate;

        bool has_mask = (ops[0].opmask != Reg::NONE);

        // Pattern: dst=reg, src1=reg, src2=reg/mem, src3=imm8
        if (dst == OperandType::Register && src1 == OperandType::Register &&
            (src3 == OperandType::Immediate || src3 == OperandType::ImmediatePlaceholder)) {

            if (src2 == OperandType::Register) {
                return has_mask ? EncodingType::EVEX_REG_REG_REG_IMM8 :
                    EncodingType::VEX_REG_REG_REG_IMM8;
            }
            if (src2 == OperandType::Memory) {
                return has_mask ? EncodingType::EVEX_REG_REG_MEM_IMM8 :
                    EncodingType::VEX_REG_REG_MEM_IMM8;
            }
        }

        // Pattern: dst=reg, src1=reg, src2=reg/mem, src3=reg (for vpblendvb)
        if (dst == OperandType::Register && src1 == OperandType::Register &&
            src3 == OperandType::Register) {

            if (src2 == OperandType::Register) {
                return EncodingType::VEX_REG_REG_REG_REG;
            }
            if (src2 == OperandType::Memory) {
                return EncodingType::VEX_REG_REG_MEM_REG;
            }
        }
    }
    return EncodingType::NO_OPERANDS;
}

constexpr const InstructionEncoding* find_instruction(std::string_view mnemonic, EncodingType type, std::string_view full_instr, const Operand* ops = nullptr, size_t count = 0) {
    // DEBUG
    //if (ops != nullptr && count > 0) {
        //printf("DEBUG find_instruction: mnemonic='%.*s', type=", (int)mnemonic.size(), mnemonic.data());
        // You'll need to call print_encoding_type here, but it's a member function
        // So just print the enum value for now
        //printf("%d, operand_count=%zu\n", (int)type, count);
    //}

    VexType desired = VexType::NONE;
    if (type == EncodingType::VEX_REG_REG_REG || type == EncodingType::VEX_REG_REG_MEM || type == EncodingType::VEX_REG_MEM ||
        type == EncodingType::EVEX_REG_REG_REG || type == EncodingType::EVEX_REG_REG_MEM) {
        desired = parser::detect_vector_size(full_instr);
    }

    // Special case: if source operand is CL, look for REG_CL/MEM_CL encoding
    EncodingType cl_type = type;
    bool has_cl_operand = false;
    if (count == 2 && ops != nullptr) {
        if (ops[1].type == OperandType::Register && ops[1].reg == Reg::CL) {
            has_cl_operand = true;
            if (type == EncodingType::REG_REG) cl_type = EncodingType::REG_CL;
            else if (type == EncodingType::MEM_REG) cl_type = EncodingType::MEM_CL;
        }
    }

    for (size_t i = 0; i < INSTRUCTION_TABLE.size(); ++i) {
        // Try CL-specific encoding first if applicable
        if (has_cl_operand && parser::str_equal(mnemonic, INSTRUCTION_TABLE[i].mnemonic) && INSTRUCTION_TABLE[i].type == cl_type) {
            if (desired != VexType::NONE) {
                if (INSTRUCTION_TABLE[i].vex_type == desired) return &INSTRUCTION_TABLE[i];
            }
            else {
                return &INSTRUCTION_TABLE[i];
            }
        }

        // Fall back to regular encoding
        if (parser::str_equal(mnemonic, INSTRUCTION_TABLE[i].mnemonic) && INSTRUCTION_TABLE[i].type == type) {
            if (desired != VexType::NONE) {
                if (INSTRUCTION_TABLE[i].vex_type == desired) return &INSTRUCTION_TABLE[i];
            }
            else {
                return &INSTRUCTION_TABLE[i];
            }
        }
    }
    return nullptr;
}

class x64Assembler {
public:
    struct JumpPatch {
        size_t instruction_pos;
        uint8_t label_id;
        uint8_t short_opcode;
        bool is_conditional;
        bool is_long;
    };
    
    explicit x64Assembler(uint64_t base_address = 0);
    
    const uint8_t* get_code() const { return buffer.get_data(); }
    size_t get_size() const { return buffer.size; }
    void print_instruction(const char* name) const;
    void print_code() const;
    size_t copy_code_to(void* dest) const;
    void reset();
    void mark_label(uint8_t label_id);
    bool finalize_jumps();
    
    void set_base_address(uint64_t new_base_address) {
        buffer.base_address = new_base_address;
    }

    uint64_t get_base_address() const {
        return buffer.base_address;
    }

    void set_output_buffer(void* ptr, size_t size_limit = 0) {
        buffer.external_buffer = static_cast<uint8_t*>(ptr);
        buffer.base_address = reinterpret_cast<uint64_t>(ptr);
        buffer.use_external = true;
        buffer.size = 0;
        // Note: You should call set_buffer_size_limit() after this to set a limit

        if (size_limit)
        {
            buffer.max_size = size_limit;
            buffer.size_limit_enabled = true;
        }
        else
        {
            buffer.size_limit_enabled = false;
            buffer.max_size = 16384;  // Reset to internal buffer size
        }
    }

    void use_internal_buffer() {
        buffer.use_external = false;
        buffer.external_buffer = nullptr;
        buffer.size = 0;
    }

    void set_buffer_size_limit(size_t limit) {
        buffer.max_size = limit;
        buffer.size_limit_enabled = true;
    }

    void disable_buffer_size_limit() {
        buffer.size_limit_enabled = false;
        buffer.max_size = 16384;  // Reset to internal buffer size
    }

    size_t get_buffer_size_limit() const {
        return buffer.max_size;
    }

    bool is_buffer_size_limit_enabled() const {
        return buffer.size_limit_enabled;
    }

    template<typename... Args>
    void emit(std::string_view instruction, Args... args) {
        pointer_table.clear();
        immediate_table.clear();
        register_table.clear();
        store_arguments(args...);

        auto [mnemonic, remaining] = parser::parse_identifier(instruction);

        // Check for size specifier
        RegSize forced_size = RegSize::DWORD;  // default for memory ops
        bool has_forced_size = false;

        remaining = parser::skip_whitespace(remaining);
        if (remaining.size() >= 4) {
            if (parser::str_equal(remaining.substr(0, 4), "byte")) {
                forced_size = RegSize::BYTE;
                has_forced_size = true;
                remaining.remove_prefix(4);
                remaining = parser::skip_whitespace(remaining);
                if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                    remaining.remove_prefix(3);
                }
            }
            else if (remaining.size() >= 4 && parser::str_equal(remaining.substr(0, 4), "word")) {
                forced_size = RegSize::WORD;
                has_forced_size = true;
                remaining.remove_prefix(4);
                remaining = parser::skip_whitespace(remaining);
                if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                    remaining.remove_prefix(3);
                }
            }
            else if (remaining.size() >= 5 && parser::str_equal(remaining.substr(0, 5), "dword")) {
                forced_size = RegSize::DWORD;
                has_forced_size = true;
                remaining.remove_prefix(5);
                remaining = parser::skip_whitespace(remaining);
                if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                    remaining.remove_prefix(3);
                }
            }
            else if (remaining.size() >= 5 && parser::str_equal(remaining.substr(0, 5), "qword")) {
                forced_size = RegSize::QWORD;
                has_forced_size = true;
                remaining.remove_prefix(5);
                remaining = parser::skip_whitespace(remaining);
                if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                    remaining.remove_prefix(3);
                }
            }
        }

        Operand operands[4];
        size_t operand_count = 0;

        while (!parser::skip_whitespace(remaining).empty() && operand_count < 4) {
            // Check for size specifier before each operand
            remaining = parser::skip_whitespace(remaining);
            RegSize operand_size = RegSize::DWORD;
            bool has_size_prefix = false;

            if (remaining.size() >= 4) {
                if (parser::str_equal(remaining.substr(0, 4), "byte")) {
                    operand_size = RegSize::BYTE;
                    has_size_prefix = true;
                    remaining.remove_prefix(4);
                    remaining = parser::skip_whitespace(remaining);
                    if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                        remaining.remove_prefix(3);
                    }
                }
                else if (remaining.size() >= 4 && parser::str_equal(remaining.substr(0, 4), "word")) {
                    operand_size = RegSize::WORD;
                    has_size_prefix = true;
                    remaining.remove_prefix(4);
                    remaining = parser::skip_whitespace(remaining);
                    if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                        remaining.remove_prefix(3);
                    }
                }
                else if (remaining.size() >= 5 && parser::str_equal(remaining.substr(0, 5), "dword")) {
                    operand_size = RegSize::DWORD;
                    has_size_prefix = true;
                    remaining.remove_prefix(5);
                    remaining = parser::skip_whitespace(remaining);
                    if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                        remaining.remove_prefix(3);
                    }
                }
                else if (remaining.size() >= 5 && parser::str_equal(remaining.substr(0, 5), "qword")) {
                    operand_size = RegSize::QWORD;
                    has_size_prefix = true;
                    remaining.remove_prefix(5);
                    remaining = parser::skip_whitespace(remaining);
                    if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                        remaining.remove_prefix(3);
                    }
                }
            }

            auto [op, after_op] = parser::parse_operand(remaining);

            // Apply size: prefer per-operand size prefix, fall back to initial forced_size
            if (has_size_prefix) {
                if (op.type == OperandType::Memory || op.type == OperandType::PointerPlaceholder) {
                    op.reg_size = operand_size;
                }
            }
            else if (has_forced_size) {
                if (op.type == OperandType::Memory || op.type == OperandType::PointerPlaceholder) {
                    op.reg_size = forced_size;
                }
            }

            operands[operand_count++] = op;
            remaining = parser::skip_comma(after_op);
            if (parser::skip_whitespace(remaining).empty()) break;
        }

        // NOTE: Removed the "Apply forced size" loop - we now handle size per-operand above

        // Resolve immediate placeholders loop
        for (size_t i = 0; i < operand_count; ++i) {
            if (operands[i].type == OperandType::ImmediatePlaceholder &&
                operands[i].placeholder_id < immediate_table.size()) {
                //printf("DEBUG: Resolving @imm%d: value=%llu, fits_in_imm8=%s\n",
                //    operands[i].placeholder_id,
                //    immediate_table[operands[i].placeholder_id],
                //    fits_in_imm8(immediate_table[operands[i].placeholder_id]) ? "YES" : "NO");
                operands[i].immediate = immediate_table[operands[i].placeholder_id];
                operands[i].type = OperandType::Immediate;
            }
        }

        // Special handling for jmp/call with pointer operands
        bool is_jump_or_call = parser::str_equal(mnemonic, "jmp") || parser::str_equal(mnemonic, "call");

        EncodingType enc_type = get_encoding_type(operands, operand_count);

        // Special handling for VEX instructions - override encoding type detection
        bool is_vex_instruction = (mnemonic.size() > 0 && parser::to_lower(mnemonic[0]) == 'v');

        // Override encoding type for VEX instructions with 2 operands
        if (is_vex_instruction && operand_count == 2) {
            if (enc_type == EncodingType::REG_REG) {
                bool has_mask = (operands[0].opmask != Reg::NONE);
                enc_type = has_mask ? EncodingType::EVEX_REG_REG : EncodingType::VEX_REG_REG;
            }
            else if (enc_type == EncodingType::REG_MEM) {
                bool has_mask = (operands[0].opmask != Reg::NONE);
                enc_type = has_mask ? EncodingType::EVEX_REG_MEM : EncodingType::VEX_REG_MEM;
            }
            else if (enc_type == EncodingType::MEM_REG) {
                bool has_mask = (operands[0].opmask != Reg::NONE);
                enc_type = has_mask ? EncodingType::EVEX_MEM_REG : EncodingType::VEX_MEM_REG;
            }
        }

        // Override encoding type for VEX instructions with 3 operands (including immediate)
        if (is_vex_instruction && operand_count == 3) {
            bool has_mask = (operands[0].opmask != Reg::NONE);

            // Check if third operand is immediate
            if (operands[2].type == OperandType::Immediate ||
                operands[2].type == OperandType::ImmediatePlaceholder) {

                if (operands[1].type == OperandType::Register ||
                    operands[1].type == OperandType::RegisterPlaceholder) {
                    enc_type = has_mask ? EncodingType::EVEX_REG_REG_IMM8 : EncodingType::VEX_REG_REG_IMM8;
                }
                else if (operands[1].type == OperandType::Memory ||
                    operands[1].type == OperandType::PointerPlaceholder) {
                    enc_type = has_mask ? EncodingType::EVEX_REG_MEM_IMM8 : EncodingType::VEX_REG_MEM_IMM8;
                }
            }
        }

        // Override encoding type for jmp/call with pointer operands
        if (is_jump_or_call && operand_count == 1) {
            if (operands[0].type == OperandType::PointerPlaceholder) {
                // Direct jump/call to absolute address via pointer
                if (parser::str_equal(mnemonic, "jmp")) {
                    enc_type = EncodingType::JUMP_ABS;
                }
                else if (parser::str_equal(mnemonic, "call")) {
                    enc_type = EncodingType::CALL_ABS;
                }
            }
            else if (operands[0].type == OperandType::Memory ||
                operands[0].type == OperandType::Register ||
                operands[0].type == OperandType::RegisterPlaceholder) {
                // Indirect jump/call through memory or register
                if (parser::str_equal(mnemonic, "jmp")) {
                    enc_type = EncodingType::JUMP_PTR;
                }
                else if (parser::str_equal(mnemonic, "call")) {
                    enc_type = EncodingType::CALL_PTR;
                }
            }
        }

        // MOV doesn't have REG_IMM8 encoding, force to REG_IMM
        if (parser::str_equal(mnemonic, "mov") && enc_type == EncodingType::REG_IMM8) {
            enc_type = EncodingType::REG_IMM;
        }

        // Force MEM_IMM when size is specified (but NOT for shift/rotate instructions that only support IMM8)
        if (has_forced_size && enc_type == EncodingType::MEM_IMM8) {
            // Check if this is a shift/rotate instruction (they only support IMM8)
            bool is_shift_rotate = parser::str_equal(mnemonic, "shl") ||
                parser::str_equal(mnemonic, "shr") ||
                parser::str_equal(mnemonic, "sal") ||
                parser::str_equal(mnemonic, "sar") ||
                parser::str_equal(mnemonic, "rol") ||
                parser::str_equal(mnemonic, "ror") ||
                parser::str_equal(mnemonic, "rcl") ||
                parser::str_equal(mnemonic, "rcr");

            if (!is_shift_rotate && (forced_size == RegSize::DWORD || forced_size == RegSize::QWORD ||
                forced_size == RegSize::WORD)) {
                enc_type = EncodingType::MEM_IMM;
            }
        }

        // Force byte size for SETcc instructions
        if (mnemonic.size() >= 3 && parser::to_lower(mnemonic[0]) == 's' &&
            parser::to_lower(mnemonic[1]) == 'e' && parser::to_lower(mnemonic[2]) == 't') {
            for (size_t i = 0; i < operand_count; ++i) {
                operands[i].reg_size = RegSize::BYTE;
            }
        }

        const auto* instr = find_instruction(mnemonic, enc_type, instruction, operands, operand_count);

        //printf("DEBUG: About to check if instr is null. mnemonic='%.*s', enc_type=%d\n",
        //    (int)mnemonic.size(), mnemonic.data(), (int)enc_type);

        if (!instr) {
            // ERROR REPORTING
            printf("ERROR: No encoding found for instruction: '%.*s'\n",
                static_cast<int>(instruction.size()), instruction.data());
            printf("  Mnemonic: '%.*s'\n",
                static_cast<int>(mnemonic.size()), mnemonic.data());
            printf("  Operand count: %zu\n", operand_count);
            printf("  Detected encoding type: ");
            print_encoding_type(enc_type);
            printf("\n");

            if (operand_count > 0) {
                printf("  Operands:\n");
                for (size_t i = 0; i < operand_count; ++i) {
                    printf("    [%zu] ", i);
                    print_operand_type(operands[i].type);
                    printf(" (size: %d-bit)\n", static_cast<int>(operands[i].reg_size));
                }
            }

            printf("\n  Possible reasons:\n");
            printf("    - This instruction form is not implemented\n");
            printf("    - Use 'dword ptr', 'qword ptr', etc. for memory operands\n");
            printf("    - Operand types are incompatible\n");
            printf("    - Check the instruction table in x64_assembler.h\n");
            return;
        }

        size_t instr_len = estimate_instruction_length(instr, operands, operand_count);
        emit_instruction(instr, operands, operand_count, instr_len);
    }

    void emit(std::string_view instruction) {
        pointer_table.clear();
        immediate_table.clear();
        register_table.clear();

        auto [mnemonic, remaining] = parser::parse_identifier(instruction);

        // Check for size specifier
        RegSize forced_size = RegSize::DWORD;  // default for memory ops
        bool has_forced_size = false;

        remaining = parser::skip_whitespace(remaining);
        if (remaining.size() >= 4) {
            if (parser::str_equal(remaining.substr(0, 4), "byte")) {
                forced_size = RegSize::BYTE;
                has_forced_size = true;
                remaining.remove_prefix(4);
                remaining = parser::skip_whitespace(remaining);
                if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                    remaining.remove_prefix(3);
                }
            }
            else if (remaining.size() >= 4 && parser::str_equal(remaining.substr(0, 4), "word")) {
                forced_size = RegSize::WORD;
                has_forced_size = true;
                remaining.remove_prefix(4);
                remaining = parser::skip_whitespace(remaining);
                if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                    remaining.remove_prefix(3);
                }
            }
            else if (remaining.size() >= 5 && parser::str_equal(remaining.substr(0, 5), "dword")) {
                forced_size = RegSize::DWORD;
                has_forced_size = true;
                remaining.remove_prefix(5);
                remaining = parser::skip_whitespace(remaining);
                if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                    remaining.remove_prefix(3);
                }
            }
            else if (remaining.size() >= 5 && parser::str_equal(remaining.substr(0, 5), "qword")) {
                forced_size = RegSize::QWORD;
                has_forced_size = true;
                remaining.remove_prefix(5);
                remaining = parser::skip_whitespace(remaining);
                if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                    remaining.remove_prefix(3);
                }
            }
        }

        Operand operands[4];
        size_t operand_count = 0;

        while (!parser::skip_whitespace(remaining).empty() && operand_count < 4) {
            // Check for size specifier before each operand
            remaining = parser::skip_whitespace(remaining);
            RegSize operand_size = RegSize::DWORD;
            bool has_size_prefix = false;

            if (remaining.size() >= 4) {
                if (parser::str_equal(remaining.substr(0, 4), "byte")) {
                    operand_size = RegSize::BYTE;
                    has_size_prefix = true;
                    remaining.remove_prefix(4);
                    remaining = parser::skip_whitespace(remaining);
                    if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                        remaining.remove_prefix(3);
                    }
                }
                else if (remaining.size() >= 4 && parser::str_equal(remaining.substr(0, 4), "word")) {
                    operand_size = RegSize::WORD;
                    has_size_prefix = true;
                    remaining.remove_prefix(4);
                    remaining = parser::skip_whitespace(remaining);
                    if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                        remaining.remove_prefix(3);
                    }
                }
                else if (remaining.size() >= 5 && parser::str_equal(remaining.substr(0, 5), "dword")) {
                    operand_size = RegSize::DWORD;
                    has_size_prefix = true;
                    remaining.remove_prefix(5);
                    remaining = parser::skip_whitespace(remaining);
                    if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                        remaining.remove_prefix(3);
                    }
                }
                else if (remaining.size() >= 5 && parser::str_equal(remaining.substr(0, 5), "qword")) {
                    operand_size = RegSize::QWORD;
                    has_size_prefix = true;
                    remaining.remove_prefix(5);
                    remaining = parser::skip_whitespace(remaining);
                    if (remaining.size() >= 3 && parser::str_equal(remaining.substr(0, 3), "ptr")) {
                        remaining.remove_prefix(3);
                    }
                }
            }

            auto [op, after_op] = parser::parse_operand(remaining);

            // Apply size: prefer per-operand size prefix, fall back to initial forced_size
            if (has_size_prefix) {
                if (op.type == OperandType::Memory || op.type == OperandType::PointerPlaceholder) {
                    op.reg_size = operand_size;
                }
            }
            else if (has_forced_size) {
                if (op.type == OperandType::Memory || op.type == OperandType::PointerPlaceholder) {
                    op.reg_size = forced_size;
                }
            }

            operands[operand_count++] = op;
            remaining = parser::skip_comma(after_op);
            if (parser::skip_whitespace(remaining).empty()) break;
        }

        // NOTE: Removed the "Apply forced size" loop - we now handle size per-operand above

        // Resolve immediate placeholders loop
        for (size_t i = 0; i < operand_count; ++i) {
            if (operands[i].type == OperandType::ImmediatePlaceholder &&
                operands[i].placeholder_id < immediate_table.size()) {
                //printf("DEBUG: Resolving @imm%d: value=%llu, fits_in_imm8=%s\n",
                //    operands[i].placeholder_id,
                //    immediate_table[operands[i].placeholder_id],
                //    fits_in_imm8(immediate_table[operands[i].placeholder_id]) ? "YES" : "NO");
                operands[i].immediate = immediate_table[operands[i].placeholder_id];
                operands[i].type = OperandType::Immediate;
            }
        }

        // Special handling for jmp/call with pointer operands
        bool is_jump_or_call = parser::str_equal(mnemonic, "jmp") || parser::str_equal(mnemonic, "call");

        EncodingType enc_type = get_encoding_type(operands, operand_count);

        // Special handling for VEX instructions - override encoding type detection
        bool is_vex_instruction = (mnemonic.size() > 0 && parser::to_lower(mnemonic[0]) == 'v');

        // Override encoding type for VEX instructions with 2 operands
        if (is_vex_instruction && operand_count == 2) {
            if (enc_type == EncodingType::REG_REG) {
                bool has_mask = (operands[0].opmask != Reg::NONE);
                enc_type = has_mask ? EncodingType::EVEX_REG_REG : EncodingType::VEX_REG_REG;
            }
            else if (enc_type == EncodingType::REG_MEM) {
                bool has_mask = (operands[0].opmask != Reg::NONE);
                enc_type = has_mask ? EncodingType::EVEX_REG_MEM : EncodingType::VEX_REG_MEM;
            }
            else if (enc_type == EncodingType::MEM_REG) {
                bool has_mask = (operands[0].opmask != Reg::NONE);
                enc_type = has_mask ? EncodingType::EVEX_MEM_REG : EncodingType::VEX_MEM_REG;
            }
        }

        // Override encoding type for VEX instructions with 3 operands (including immediate)
        if (is_vex_instruction && operand_count == 3) {
            bool has_mask = (operands[0].opmask != Reg::NONE);

            // Check if third operand is immediate
            if (operands[2].type == OperandType::Immediate ||
                operands[2].type == OperandType::ImmediatePlaceholder) {

                if (operands[1].type == OperandType::Register ||
                    operands[1].type == OperandType::RegisterPlaceholder) {
                    enc_type = has_mask ? EncodingType::EVEX_REG_REG_IMM8 : EncodingType::VEX_REG_REG_IMM8;
                }
                else if (operands[1].type == OperandType::Memory ||
                    operands[1].type == OperandType::PointerPlaceholder) {
                    enc_type = has_mask ? EncodingType::EVEX_REG_MEM_IMM8 : EncodingType::VEX_REG_MEM_IMM8;
                }
            }
        }

        // Override encoding type for jmp/call with pointer operands
        if (is_jump_or_call && operand_count == 1) {
            if (operands[0].type == OperandType::PointerPlaceholder) {
                // Direct jump/call to absolute address via pointer
                if (parser::str_equal(mnemonic, "jmp")) {
                    enc_type = EncodingType::JUMP_ABS;
                }
                else if (parser::str_equal(mnemonic, "call")) {
                    enc_type = EncodingType::CALL_ABS;
                }
            }
            else if (operands[0].type == OperandType::Memory ||
                operands[0].type == OperandType::Register ||
                operands[0].type == OperandType::RegisterPlaceholder) {
                // Indirect jump/call through memory or register
                if (parser::str_equal(mnemonic, "jmp")) {
                    enc_type = EncodingType::JUMP_PTR;
                }
                else if (parser::str_equal(mnemonic, "call")) {
                    enc_type = EncodingType::CALL_PTR;
                }
            }
        }

        // MOV doesn't have REG_IMM8 encoding, force to REG_IMM
        if (parser::str_equal(mnemonic, "mov") && enc_type == EncodingType::REG_IMM8) {
            enc_type = EncodingType::REG_IMM;
        }

        // Force MEM_IMM when size is specified (but NOT for shift/rotate instructions that only support IMM8)
        if (has_forced_size && enc_type == EncodingType::MEM_IMM8) {
            // Check if this is a shift/rotate instruction (they only support IMM8)
            bool is_shift_rotate = parser::str_equal(mnemonic, "shl") ||
                parser::str_equal(mnemonic, "shr") ||
                parser::str_equal(mnemonic, "sal") ||
                parser::str_equal(mnemonic, "sar") ||
                parser::str_equal(mnemonic, "rol") ||
                parser::str_equal(mnemonic, "ror") ||
                parser::str_equal(mnemonic, "rcl") ||
                parser::str_equal(mnemonic, "rcr");

            if (!is_shift_rotate && (forced_size == RegSize::DWORD || forced_size == RegSize::QWORD ||
                forced_size == RegSize::WORD)) {
                enc_type = EncodingType::MEM_IMM;
            }
        }

        // Force byte size for SETcc instructions
        if (mnemonic.size() >= 3 && parser::to_lower(mnemonic[0]) == 's' &&
            parser::to_lower(mnemonic[1]) == 'e' && parser::to_lower(mnemonic[2]) == 't') {
            for (size_t i = 0; i < operand_count; ++i) {
                operands[i].reg_size = RegSize::BYTE;
            }
        }

        const auto* instr = find_instruction(mnemonic, enc_type, instruction, operands, operand_count);
        if (!instr) {
            // ERROR REPORTING
            printf("ERROR: No encoding found for instruction: '%.*s'\n",
                static_cast<int>(instruction.size()), instruction.data());
            printf("  Mnemonic: '%.*s'\n",
                static_cast<int>(mnemonic.size()), mnemonic.data());
            printf("  Operand count: %zu\n", operand_count);
            printf("  Detected encoding type: ");
            print_encoding_type(enc_type);
            printf("\n");

            if (operand_count > 0) {
                printf("  Operands:\n");
                for (size_t i = 0; i < operand_count; ++i) {
                    printf("    [%zu] ", i);
                    print_operand_type(operands[i].type);
                    printf(" (size: %d-bit)\n", static_cast<int>(operands[i].reg_size));
                }
            }

            printf("\n  Possible reasons:\n");
            printf("    - This instruction form is not implemented\n");
            printf("    - Use 'dword ptr', 'qword ptr', etc. for memory operands\n");
            printf("    - Operand types are incompatible\n");
            printf("    - Check the instruction table in x64_assembler.h\n");
            return;
        }

        size_t instr_len = estimate_instruction_length(instr, operands, operand_count);
        emit_instruction(instr, operands, operand_count, instr_len);
    }

private:
    static void print_encoding_type(EncodingType type) {
        switch (type) {
        case EncodingType::REG_REG: printf("REG_REG"); break;
        case EncodingType::REG_IMM: printf("REG_IMM"); break;
        case EncodingType::REG_IMM8: printf("REG_IMM8"); break;
        case EncodingType::REG_MEM: printf("REG_MEM"); break;
        case EncodingType::MEM_REG: printf("MEM_REG"); break;
        case EncodingType::MEM_IMM: printf("MEM_IMM"); break;
        case EncodingType::MEM_IMM8: printf("MEM_IMM8"); break;
        case EncodingType::SINGLE_REG: printf("SINGLE_REG"); break;
        case EncodingType::NO_OPERANDS: printf("NO_OPERANDS"); break;
        case EncodingType::ACC_IMM: printf("ACC_IMM"); break;
        case EncodingType::VEX_REG_REG_IMM8: printf("VEX_REG_REG_IMM8"); break;
        case EncodingType::VEX_REG_REG_REG: printf("VEX_REG_REG_REG"); break;
        case EncodingType::VEX_REG_REG_MEM: printf("VEX_REG_REG_MEM"); break;
        case EncodingType::VEX_REG_MEM: printf("VEX_REG_MEM"); break;
        case EncodingType::VEX_REG_REG: printf("VEX_REG_REG"); break;
        case EncodingType::EVEX_REG_REG_IMM8: printf("EVEX_REG_REG_IMM8"); break;
        case EncodingType::EVEX_REG_REG_REG: printf("EVEX_REG_REG_REG"); break;
        case EncodingType::EVEX_REG_REG_MEM: printf("EVEX_REG_REG_MEM"); break;
        case EncodingType::VEX_MEM_REG_IMM8: printf("VEX_MEM_REG_IMM8"); break;
        case EncodingType::EVEX_MEM_REG_IMM8: printf("EVEX_MEM_REG_IMM8"); break;
        case EncodingType::JUMP_LABEL: printf("JUMP_LABEL"); break;
        case EncodingType::LONG_JUMP_LABEL: printf("LONG_JUMP_LABEL"); break;
        case EncodingType::SINGLE_MEM: printf("SINGLE_MEM"); break;
        case EncodingType::REG_CL: printf("REG_CL"); break;
        case EncodingType::MEM_CL: printf("MEM_CL"); break;
        case EncodingType::REG_REG_IMM: printf("REG_REG_IMM"); break;
        case EncodingType::REG_REG_IMM8: printf("REG_REG_IMM8"); break;
        case EncodingType::REG_MEM_IMM: printf("REG_MEM_IMM"); break;
        case EncodingType::REG_MEM_IMM8: printf("REG_MEM_IMM8"); break;
        case EncodingType::JUMP_PTR: printf("JUMP_PTR"); break;
        case EncodingType::CALL_PTR: printf("CALL_PTR"); break;
        case EncodingType::JUMP_ABS: printf("JUMP_ABS"); break;
        case EncodingType::CALL_ABS: printf("CALL_ABS"); break;
        default: printf("UNKNOWN"); break;
        }
    }

    static void print_operand_type(OperandType type) {
        switch (type) {
        case OperandType::Register: printf("Register"); break;
        case OperandType::Immediate: printf("Immediate"); break;
        case OperandType::Memory: printf("Memory"); break;
        case OperandType::PointerPlaceholder: printf("PointerPlaceholder (@ptr)"); break;
        case OperandType::ImmediatePlaceholder: printf("ImmediatePlaceholder (@imm)"); break;
        case OperandType::RegisterPlaceholder: printf("RegisterPlaceholder (@reg)"); break;
        case OperandType::JumpLabel: printf("JumpLabel (@jmp)"); break;
        case OperandType::LongJumpLabel: printf("LongJumpLabel (@longjmp)"); break;
        default: printf("Unknown"); break;
        }
    }

    RuntimeInstructionBuffer<16384> buffer;
    std::vector<void*> pointer_table;
    std::vector<uint64_t> immediate_table;
    std::vector<Reg> register_table;
    std::vector<JumpPatch> jump_patches;
    uint64_t label_positions[16];
    bool label_defined[16];
    
    bool calculate_rip_offset(void* ptr, size_t len, int32_t& out_offset) const;
    bool calculate_absolute_offset(void* ptr, size_t instr_len, int32_t& out_offset) const;
    Reg resolve_register(const Operand& op) const;
    size_t estimate_instruction_length(const InstructionEncoding* instr, const Operand* ops, size_t count) const;
    
    void store_arguments() {}
    
    template<typename T, typename... Rest>
    void store_arguments(T first, Rest... rest) {
        if constexpr (std::is_pointer_v<T>) {
            // Remove const and volatile qualifiers before casting to void*
            using BaseType = std::remove_cv_t<std::remove_pointer_t<T>>;
            pointer_table.push_back(static_cast<void*>(const_cast<BaseType*>(first)));
        }
        else if constexpr (std::is_same_v<T, Reg>) {
            register_table.push_back(first);
        }
        else if constexpr (std::is_integral_v<T>) {
            immediate_table.push_back(static_cast<uint64_t>(first));
        }
        store_arguments(rest...);
    }

    void emit_vex2(uint8_t pp, uint8_t vvvv, bool l, uint8_t rex_r, uint8_t op);
    void emit_vex3(uint8_t mmmmm, uint8_t pp, uint8_t vvvv, bool l, bool w, uint8_t op);
    void emit_evex(uint8_t mmmmm, uint8_t pp, uint8_t vvvv, bool w, uint8_t op, uint8_t ll, uint8_t aaa, bool z);
    void emit_instruction(const InstructionEncoding* instr, const Operand* ops, size_t count, size_t est_len);
    void emit_vex_instruction(const InstructionEncoding* instr, const Operand* ops, size_t count);
    void emit_rip_relative_modrm(Reg reg_field, uint8_t ptr_id, size_t len);
    void emit_memory_operand(Reg reg_field, const MemoryOperand& mem);
    uint8_t encode_sib(const MemoryOperand& mem) const;
    uint8_t get_rex_prefix(const Operand* ops, size_t count, RegSize size) const;
};

} // namespace x64asm

#endif // X64_ASSEMBLER_H