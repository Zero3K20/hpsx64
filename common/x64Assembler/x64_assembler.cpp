// x64_assembler.cpp
// MSVC C++17 compatible implementation with multi-size support

#include "x64_assembler.h"
#include <cstdio>
#include <cstring>

namespace x64asm {

template struct RuntimeInstructionBuffer<16384>;

x64Assembler::x64Assembler(uint64_t base_address) : buffer(base_address) {
    for (int i = 0; i < 16; ++i) {
        label_positions[i] = 0;
        label_defined[i] = false;
    }
}

void x64Assembler::print_instruction(const char* name) const {
    printf("%s: ", name);
    for (size_t i = 0; i < buffer.size; ++i) printf("%02X ", buffer.data[i]);
    printf("(%zu bytes)\n", buffer.size);
}

void x64Assembler::print_code() const {
    if (buffer.size == 0) {
        printf("(empty - 0 bytes)\n");
        return;
    }
    
    printf("Generated code (%zu bytes):\n", buffer.size);
    
    // Print in rows of 16 bytes with address offsets
    for (size_t i = 0; i < buffer.size; i += 16) {
        printf("%08llX: ", static_cast<unsigned long long>(buffer.base_address + i));
        
        // Print hex bytes
        for (size_t j = 0; j < 16; ++j) {
            if (i + j < buffer.size) {
                printf("%02X ", buffer.data[i + j]);
            } else {
                printf("   ");
            }
            
            // Extra space after 8 bytes for readability
            if (j == 7) printf(" ");
        }
        
        printf("\n");
    }
}

size_t x64Assembler::copy_code_to(void* dest) const {
    if (dest != nullptr && buffer.size > 0) {
        std::memcpy(dest, buffer.data.data(), buffer.size);
        return buffer.size;
    }
    return 0;
}

void x64Assembler::reset() {
    buffer.size = 0;
    // Don't reset external buffer settings
    pointer_table.clear();
    immediate_table.clear();
    register_table.clear();
    jump_patches.clear();
    for (int i = 0; i < 16; ++i) {
        label_positions[i] = 0;
        label_defined[i] = false;
    }
}

bool x64Assembler::calculate_rip_offset(void* ptr, size_t len, int32_t& out_offset) const {
    uint64_t target = reinterpret_cast<uint64_t>(ptr);
    uint64_t next = buffer.current_address() + len;
    int64_t offset = static_cast<int64_t>(target - next);

    // Check if offset fits in 32-bit signed range
    if (offset < -2147483648LL || offset > 2147483647LL) {
        printf("ERROR: RIP-relative offset out of range!\n");
        printf("  Instruction address: 0x%016llX\n", static_cast<unsigned long long>(buffer.current_address()));
        printf("  Target address:      0x%016llX\n", static_cast<unsigned long long>(target));
        printf("  Distance:            %lld bytes (0x%llX)\n", offset, static_cast<unsigned long long>(offset));
        printf("  Maximum range:       +/- 2GB (32-bit signed offset)\n");
        return false;
    }

    out_offset = static_cast<int32_t>(offset);
    return true;
}

bool x64Assembler::calculate_absolute_offset(void* ptr, size_t instr_len, int32_t& out_offset) const {
    uint64_t target = reinterpret_cast<uint64_t>(ptr);
    uint64_t next = buffer.current_address() + instr_len;
    int64_t offset = static_cast<int64_t>(target - next);

    // Check if offset fits in 32-bit signed range
    if (offset < -2147483648LL || offset > 2147483647LL) {
        printf("ERROR: Direct jump/call offset out of range!\n");
        printf("  Instruction address: 0x%016llX\n", static_cast<unsigned long long>(buffer.current_address()));
        printf("  Target address:      0x%016llX\n", static_cast<unsigned long long>(target));
        printf("  Distance:            %lld bytes (0x%llX)\n", offset, static_cast<unsigned long long>(offset));
        printf("  Maximum range:       +/- 2GB (32-bit signed offset)\n");
        printf("  Note: Use indirect jump through memory if target is too far\n");
        return false;
    }

    out_offset = static_cast<int32_t>(offset);
    return true;
}

void x64Assembler::mark_label(uint8_t label_id) {
    if (label_id < 16) {
        label_positions[label_id] = buffer.current_address();
        label_defined[label_id] = true;
    }
}

Reg x64Assembler::resolve_register(const Operand& op) const {
    if (op.type == OperandType::RegisterPlaceholder) {
        if (op.placeholder_id < register_table.size()) {
            return register_table[op.placeholder_id];
        }
        return Reg::RAX;
    }
    return op.reg;
}

uint8_t x64Assembler::get_rex_prefix(const Operand* ops, size_t count, RegSize size) const {
    uint8_t rex = 0x40;
    bool needs_rex = false;

    // REX.W for 64-bit operations - only if instruction supports it
    // (Don't add this automatically for NO_OPERANDS instructions like ret)
    if (size == RegSize::QWORD && count > 0) {  // <-- Add count > 0 check
        rex |= 0x08; // Set REX.W
        needs_rex = true;
    }

    // Check if we need REX for extended registers or special byte registers
    for (size_t i = 0; i < count; ++i) {
        if (ops[i].type == OperandType::Register || ops[i].type == OperandType::RegisterPlaceholder) {
            Reg r = resolve_register(ops[i]);
            uint8_t reg_num = static_cast<uint8_t>(r);
            
            // Extended registers (R8-R15) need REX
            if (reg_num >= 8 && reg_num <= 15) {
                needs_rex = true;
                // Set REX.B for first operand or REX.R for second operand
                if (i == 0) rex |= 0x01; // REX.B (ModRM r/m field)
                else if (i == 1) rex |= 0x04; // REX.R (ModRM reg field)
            }
            
            // For byte operations with SPL, BPL, SIL, DIL (need REX to access)
            if (size == RegSize::BYTE && reg_num >= 4 && reg_num <= 7) {
                needs_rex = true;
            }
        }
    }
    
    return needs_rex ? rex : 0;
}

bool x64Assembler::finalize_jumps() {
    for (const auto& patch : jump_patches) {
        if (patch.label_id >= 16 || !label_defined[patch.label_id]) {
            printf("Error: Jump label @%sjmp%d not defined\n", patch.is_long ? "long" : "", patch.label_id);
            return false;
        }
        
        uint64_t target_addr = label_positions[patch.label_id];
        size_t jump_size = patch.is_long ? (patch.is_conditional ? 6 : 5) : 2;
        uint64_t jump_end_addr = buffer.base_address + patch.instruction_pos + jump_size;
        int64_t offset = static_cast<int64_t>(target_addr - jump_end_addr);
        
        if (patch.is_long) {
            if (offset < -2147483648LL || offset > 2147483647LL) {
                printf("Error: Long jump @longjmp%d is out of range (> 2GB)\n", patch.label_id);
                printf("       Distance: %lld bytes\n", offset);
                return false;
            }
            
            if (patch.is_conditional) {
                buffer.data[patch.instruction_pos] = 0x0F;
                buffer.data[patch.instruction_pos + 1] = patch.short_opcode + 0x10;
                uint32_t offset32 = static_cast<uint32_t>(offset);
                buffer.data[patch.instruction_pos + 2] = uint8_t(offset32);
                buffer.data[patch.instruction_pos + 3] = uint8_t(offset32 >> 8);
                buffer.data[patch.instruction_pos + 4] = uint8_t(offset32 >> 16);
                buffer.data[patch.instruction_pos + 5] = uint8_t(offset32 >> 24);
            } else {
                buffer.data[patch.instruction_pos] = 0xE9;
                uint32_t offset32 = static_cast<uint32_t>(offset);
                buffer.data[patch.instruction_pos + 1] = uint8_t(offset32);
                buffer.data[patch.instruction_pos + 2] = uint8_t(offset32 >> 8);
                buffer.data[patch.instruction_pos + 3] = uint8_t(offset32 >> 16);
                buffer.data[patch.instruction_pos + 4] = uint8_t(offset32 >> 24);
            }
        } else {
            if (offset < -128 || offset > 127) {
                printf("Error: Short jump @jmp%d is out of range for short encoding\n", patch.label_id);
                printf("       Distance: %lld bytes (must be -128 to +127)\n", offset);
                printf("       Use @longjmp%d instead for this distance\n", patch.label_id);
                return false;
            }
            buffer.data[patch.instruction_pos + 1] = static_cast<uint8_t>(offset);
        }
    }
    return true;
}

size_t x64Assembler::estimate_instruction_length(const InstructionEncoding* instr, const Operand* ops, size_t count) const {
    size_t len = 0;
    
    // VEX/EVEX instructions have fixed structure
    if (instr->vex_type == VexType::EVEX_128 || instr->vex_type == VexType::EVEX_256 || instr->vex_type == VexType::EVEX_512) {
        len = 6;
        for (size_t i = 0; i < count; ++i)
            if (ops[i].type == OperandType::PointerPlaceholder) { len += 4; break; }
        return len;
    }
    if (instr->vex_type == VexType::VEX_128 || instr->vex_type == VexType::VEX_256) {
        len = 5;
        for (size_t i = 0; i < count; ++i)
            if (ops[i].type == OperandType::PointerPlaceholder) { len += 4; break; }
        return len;
    }
    
    RegSize size = get_operand_size(ops, count);
    
    if (size == RegSize::WORD) len++;
    if (instr->prefix) len++;
    
    uint8_t rex = get_rex_prefix(ops, count, size);
    if (rex != 0) len++;
    
    if (instr->is_two_byte) len += 2;
    else len++;
    
    if (instr->has_modrm) len++;
    
    for (size_t i = 0; i < count; ++i)
        if (ops[i].type == OperandType::PointerPlaceholder) { len += 4; break; }
    
    if (instr->type == EncodingType::REG_IMM) {
        if (parser::str_equal(instr->mnemonic, "mov")) {
            if (size == RegSize::QWORD) len += 8;
            else if (size == RegSize::DWORD) len += 4;
            else if (size == RegSize::WORD) len += 2;
            else len += 1;
        } else {
            if (size == RegSize::QWORD || size == RegSize::DWORD) len += 4;
            else if (size == RegSize::WORD) len += 2;
            else len += 1;
        }
    } else if (instr->type == EncodingType::REG_IMM8 || instr->type == EncodingType::MEM_IMM8) {
        len += 1;
    } else if (instr->type == EncodingType::MEM_IMM) {
        if (size == RegSize::QWORD || size == RegSize::DWORD) len += 4;
        else if (size == RegSize::WORD) len += 2;
        else len += 1;
    } else if (instr->type == EncodingType::ACC_IMM) {
        if (size == RegSize::QWORD || size == RegSize::DWORD) len += 4;
        else if (size == RegSize::WORD) len += 2;
        else len += 1;
    }
    
    return len;
}

void x64Assembler::emit_vex2(uint8_t pp, uint8_t vvvv, bool l, uint8_t rex_r, uint8_t op) {
    // 2-byte VEX: C5 [R vvvv L pp] opcode
    // R bit is inverted REX.R
    buffer.emit(0xC5);
    buffer.emit((rex_r ? 0x00 : 0x80) | ((~vvvv & 0x0F) << 3) | (l ? 0x04 : 0) | (pp & 0x03));
    buffer.emit(op);
}

void x64Assembler::emit_vex3(uint8_t mmmmm, uint8_t pp, uint8_t vvvv, bool l, bool w, uint8_t op) {
    buffer.emit(0xC4);
    buffer.emit((mmmmm & 0x1F) | 0xE0);
    buffer.emit((w ? 0x80 : 0) | ((~vvvv & 0x0F) << 3) | (l ? 0x04 : 0) | (pp & 0x03));
    buffer.emit(op);
}

void x64Assembler::emit_evex(uint8_t mmmmm, uint8_t pp, uint8_t vvvv, bool w, uint8_t op, uint8_t ll, uint8_t aaa, bool z) {
    buffer.emit(0x62);
    buffer.emit((mmmmm & 0x03) | 0xF0);
    buffer.emit((w ? 0x80 : 0) | ((~vvvv & 0x0F) << 3) | 0x04 | (pp & 0x03));
    buffer.emit((z ? 0x80 : 0) | ((ll & 0x03) << 5) | 0x08 | ((~vvvv >> 4) & 0x08) | (aaa & 0x07));
    buffer.emit(op);
}

void x64Assembler::emit_vex_instruction(const InstructionEncoding* instr, const Operand* ops, size_t count) {
    uint8_t vvvv = 0x0;  // Default: no second source (all 1s = unused)

    // For 3-operand and 4-operand instructions, encode the second operand in vvvv
    // Pattern: dst, src1, src2, [imm/mask] where src1 (ops[1]) goes in vvvv
    if (count >= 3 && (ops[1].type == OperandType::Register || ops[1].type == OperandType::RegisterPlaceholder)) {
        vvvv = static_cast<uint8_t>(resolve_register(ops[1])) & 0xF;
    }

    uint8_t opmask_reg = 0;
    bool zero_masking = false;
    if (count > 0 && ops[0].opmask != Reg::NONE) {
        opmask_reg = static_cast<uint8_t>(ops[0].opmask) & 0x07;
        zero_masking = (ops[0].mask_mode == MaskMode::Zero);
    }

    if (instr->vex_type == VexType::EVEX_128 || instr->vex_type == VexType::EVEX_256 || instr->vex_type == VexType::EVEX_512) {
        uint8_t ll = (instr->vex_type == VexType::EVEX_128) ? 0x00 : (instr->vex_type == VexType::EVEX_256) ? 0x01 : 0x02;
        emit_evex(instr->vex_map, instr->prefix, vvvv, instr->vex_w, instr->opcode, ll, opmask_reg, zero_masking);
    }
    else {
        // Calculate REX bits from operands
        uint8_t rex_r = 0, rex_x = 0, rex_b = 0;

        // Check destination register (first operand)
        if (count > 0 && (ops[0].type == OperandType::Register || ops[0].type == OperandType::RegisterPlaceholder)) {
            uint8_t reg_num = static_cast<uint8_t>(resolve_register(ops[0]));
            if (reg_num >= 8) rex_r = 1;  // Extended register in ModRM reg field
        }

        // Check source register (third operand for both 3-op and 4-op forms)
        // For 4-operand: dst, src1, src2, imm/mask  -> check ops[2]
        // For 3-operand: dst, src1, src2            -> check ops[2]
        // For 2-operand: dst, src                   -> check ops[1]
        size_t src_idx = (count >= 3) ? 2 : (count == 2) ? 1 : 0;

        if (src_idx < count && ops[src_idx].type != OperandType::Immediate &&
            ops[src_idx].type != OperandType::ImmediatePlaceholder) {
            if (ops[src_idx].type == OperandType::Register || ops[src_idx].type == OperandType::RegisterPlaceholder) {
                uint8_t reg_num = static_cast<uint8_t>(resolve_register(ops[src_idx]));
                if (reg_num >= 8) rex_b = 1;  // Extended register in ModRM r/m field
            }
            else if (ops[src_idx].type == OperandType::Memory) {
                const auto& mem = ops[src_idx].memory;
                if (mem.has_base && static_cast<uint8_t>(mem.base) >= 8) rex_b = 1;
                if (mem.has_index && static_cast<uint8_t>(mem.index) >= 8) rex_x = 1;
            }
        }

        bool is_256 = (instr->vex_type == VexType::VEX_256);

        if (can_use_2byte_vex(instr->vex_map, rex_r, rex_x, rex_b, instr->vex_w)) {
            emit_vex2(instr->prefix, vvvv, is_256, rex_r, instr->opcode);
        }
        else {
            emit_vex3(instr->vex_map, instr->prefix, vvvv, is_256, instr->vex_w, instr->opcode);
        }
    }

    // =========================================================================
    // NOW ADD THE 4-OPERAND HANDLING IN THE ModRM SECTION
    // This comes AFTER the VEX/EVEX prefix emission above
    // =========================================================================

    // Emit ModRM byte and operands
    if (count == 4) {
        // 4-operand form: dst, src1, src2, imm8/mask_reg
        bool has_immediate = (ops[3].type == OperandType::Immediate ||
            ops[3].type == OperandType::ImmediatePlaceholder);

        if (ops[2].type == OperandType::PointerPlaceholder) {
            size_t bytes_after = has_immediate ? 5 : 4;  // 4-byte offset + optional imm8
            emit_rip_relative_modrm(resolve_register(ops[0]), ops[2].placeholder_id, bytes_after);
        }
        else if (ops[2].type == OperandType::Memory) {
            emit_memory_operand(resolve_register(ops[0]), ops[2].memory);
        }
        else {
            // Register to register
            buffer.emit(0xC0 | (uint8_t(resolve_register(ops[0])) << 3) | uint8_t(resolve_register(ops[2])));
        }

        // Emit the 4th operand
        if (has_immediate) {
            // Immediate byte
            uint8_t imm = uint8_t(ops[3].immediate);
            if (ops[3].type == OperandType::ImmediatePlaceholder && ops[3].placeholder_id < immediate_table.size())
                imm = uint8_t(immediate_table[ops[3].placeholder_id]);
            buffer.emit(imm);
        }
        else if (ops[3].type == OperandType::Register || ops[3].type == OperandType::RegisterPlaceholder) {
            // For vpblendvb-style: 4th operand encoded in immediate byte (bits 4-7)
            uint8_t mask_reg = static_cast<uint8_t>(resolve_register(ops[3])) & 0x0F;
            buffer.emit(mask_reg << 4);
        }
        return;
    }

    // Emit ModRM byte
    if (count == 2) {
        // Check if this is a store (MEM_REG) or load (REG_MEM)
        bool is_store = (ops[0].type == OperandType::Memory || ops[0].type == OperandType::PointerPlaceholder);

        if (is_store) {
            // Store: vmovdqa [mem], xmm
            if (ops[0].type == OperandType::PointerPlaceholder)
                emit_rip_relative_modrm(resolve_register(ops[1]), ops[0].placeholder_id, 4);
            else emit_memory_operand(resolve_register(ops[1]), ops[0].memory);
        }
        else if (ops[1].type == OperandType::Memory || ops[1].type == OperandType::PointerPlaceholder) {
            // Load: vmovdqa xmm, [mem]
            if (ops[1].type == OperandType::PointerPlaceholder)
                emit_rip_relative_modrm(resolve_register(ops[0]), ops[1].placeholder_id, 4);
            else emit_memory_operand(resolve_register(ops[0]), ops[1].memory);
        }
        else {
            // Register to register
            buffer.emit(0xC0 | (uint8_t(resolve_register(ops[0])) << 3) | uint8_t(resolve_register(ops[1])));
        }
    }
    else if (count == 3) {
        // Handle 2-operand + immediate instructions like vpshufd
        if (instr->type == EncodingType::VEX_REG_MEM_IMM8 || instr->type == EncodingType::EVEX_REG_MEM_IMM8 ||
            instr->type == EncodingType::VEX_REG_REG_IMM8 || instr->type == EncodingType::EVEX_REG_REG_IMM8) {
            // 2-operand + immediate form: dst, src, imm
            if (ops[1].type == OperandType::Memory || ops[1].type == OperandType::PointerPlaceholder) {
                // For memory operands, check if this uses reg_field or destination register
                // Shift instructions (reg_field != 0) use reg_field, others use destination register
                Reg modrm_reg = (instr->reg_field != 0) ? static_cast<Reg>(instr->reg_field) : resolve_register(ops[0]);

                if (ops[1].type == OperandType::PointerPlaceholder) {
                    emit_rip_relative_modrm(modrm_reg, ops[1].placeholder_id, 4 + 1);
                }
                else {
                    emit_memory_operand(modrm_reg, ops[1].memory);
                }
            }
            else {
                // Register to register
                // Shift instructions (reg_field != 0) use reg_field, others use destination register
                if (instr->reg_field != 0) {
                    // Shift/rotate instructions with immediate
                    buffer.emit(0xC0 | (instr->reg_field << 3) | uint8_t(resolve_register(ops[1])));
                }
                else {
                    // Other instructions like vpshufd
                    buffer.emit(0xC0 | (uint8_t(resolve_register(ops[0])) << 3) | uint8_t(resolve_register(ops[1])));
                }
            }
            // Emit immediate byte
            uint8_t imm = uint8_t(ops[2].immediate);
            if (ops[2].type == OperandType::ImmediatePlaceholder && ops[2].placeholder_id < immediate_table.size())
                imm = uint8_t(immediate_table[ops[2].placeholder_id]);
            buffer.emit(imm);
            return;
        }

        // Check if third operand is immediate
        if (ops[2].type == OperandType::Immediate || ops[2].type == OperandType::ImmediatePlaceholder) {
            // VEX/EVEX with immediate (e.g., vpsllw xmm0, xmm1, imm8)
            // Encode using reg_field from instruction
            if (ops[1].type == OperandType::Memory || ops[1].type == OperandType::PointerPlaceholder) {
                if (ops[1].type == OperandType::PointerPlaceholder) {
                    emit_rip_relative_modrm(static_cast<Reg>(instr->reg_field), ops[1].placeholder_id, 4 + 1); // offset + imm8
                }
                else {
                    emit_memory_operand(static_cast<Reg>(instr->reg_field), ops[1].memory);
                }
            }
            else {
                // Register form
                buffer.emit(0xC0 | (instr->reg_field << 3) | uint8_t(resolve_register(ops[1])));
            }
            // Emit immediate byte
            uint8_t imm = uint8_t(ops[2].immediate);
            if (ops[2].type == OperandType::ImmediatePlaceholder && ops[2].placeholder_id < immediate_table.size())
                imm = uint8_t(immediate_table[ops[2].placeholder_id]);
            buffer.emit(imm);
        }
        else if (ops[2].type == OperandType::Memory || ops[2].type == OperandType::PointerPlaceholder) {
            if (ops[2].type == OperandType::PointerPlaceholder)
                emit_rip_relative_modrm(resolve_register(ops[0]), ops[2].placeholder_id, 4);
            else emit_memory_operand(resolve_register(ops[0]), ops[2].memory);
        }
        else {
            buffer.emit(0xC0 | (uint8_t(resolve_register(ops[0])) << 3) | uint8_t(resolve_register(ops[2])));
        }
    }
}

void x64Assembler::emit_instruction(const InstructionEncoding* instr, const Operand* ops, size_t count, size_t est_len) {
    // Handle VEX/EVEX instructions
    if (instr->vex_type != VexType::NONE) { 
        emit_vex_instruction(instr, ops, count); 
        return; 
    }
    
    // Handle jump instructions
    if ((instr->type == EncodingType::JUMP_LABEL || instr->type == EncodingType::LONG_JUMP_LABEL) && 
        count == 1 && (ops[0].type == OperandType::JumpLabel || ops[0].type == OperandType::LongJumpLabel)) {
        
        bool is_long = (ops[0].type == OperandType::LongJumpLabel);
        bool is_conditional = !parser::str_equal(instr->mnemonic, "jmp");
        
        JumpPatch patch;
        patch.instruction_pos = buffer.size;
        patch.label_id = ops[0].placeholder_id;
        patch.short_opcode = instr->opcode;
        patch.is_conditional = is_conditional;
        patch.is_long = is_long;
        jump_patches.push_back(patch);
        
        if (is_long) {
            if (is_conditional) {
                buffer.emit(0x0F);
                buffer.emit(instr->opcode + 0x10);
                buffer.emit_dword(0x00000000);
            } else {
                buffer.emit(0xE9);
                buffer.emit_dword(0x00000000);
            }
        } else {
            buffer.emit(instr->opcode);
            buffer.emit(0x00);
        }
        return;
    }
    
    // Determine operand size from registers
    RegSize size = get_operand_size(ops, count);

    // Special handling for CWD/CDQ/CQO - determine size from mnemonic
    if (parser::str_equal(instr->mnemonic, "cwd")) {
        size = RegSize::WORD;
    }
    else if (parser::str_equal(instr->mnemonic, "cdq")) {
        size = RegSize::DWORD;
    }
    else if (parser::str_equal(instr->mnemonic, "cqo")) {
        size = RegSize::QWORD;
    }

    // Special handling for CBW/CWDE/CDQE - determine size from mnemonic
    if (parser::str_equal(instr->mnemonic, "cbw")) {
        size = RegSize::WORD;
    }
    else if (parser::str_equal(instr->mnemonic, "cwde")) {
        size = RegSize::DWORD;
    }
    else if (parser::str_equal(instr->mnemonic, "cdqe")) {
        size = RegSize::QWORD;
    }

    // === EMIT PREFIXES (in correct order) ===
    
    // 1. Operand size prefix (16-bit)
    if (size == RegSize::WORD) {
        buffer.emit(0x66);
    }
    
    // 2. Legacy prefixes
    if (instr->prefix) {
        buffer.emit(instr->prefix);
    }
    
    // 3. REX prefix (must be last prefix before opcode)
    uint8_t rex = get_rex_prefix(ops, count, size);

    // Special case: CQO and CDQE need REX.W even though they have no operands
    if (size == RegSize::QWORD && (parser::str_equal(instr->mnemonic, "cqo") ||
        parser::str_equal(instr->mnemonic, "cdqe"))) {
        rex = 0x48;  // REX.W
    }

    if (rex != 0) {
        buffer.emit(rex);
    }
    
    // === EMIT OPCODE ===
    
    // Adjust opcodes for byte operations
    uint8_t opcode = instr->opcode;
    if (size == RegSize::BYTE && !instr->is_two_byte) {
        // Most byte operations use opcode with bit 0 cleared
        if (opcode == 0x89 || opcode == 0x8B) opcode = 0x88;
        else if (opcode == 0x01 || opcode == 0x03) opcode &= ~1;
        else if (opcode == 0x29 || opcode == 0x2B) opcode &= ~1;
        else if (opcode == 0x21 || opcode == 0x23) opcode &= ~1;
        else if (opcode == 0x09 || opcode == 0x0B) opcode &= ~1;
        else if (opcode == 0x31 || opcode == 0x33) opcode &= ~1;
        else if (opcode == 0x39 || opcode == 0x3B) opcode &= ~1;
        else if (opcode == 0x81) opcode = 0x80;
        else if (opcode == 0xC7) opcode = 0xC6;
    }
    
    // Emit opcode(s)
    if (instr->is_two_byte) {
        buffer.emit(0x0F);

        // Special handling for MOVZX/MOVSX - opcode depends on source size
        uint8_t opcode2 = instr->opcode2;
        if ((parser::str_equal(instr->mnemonic, "movzx") || parser::str_equal(instr->mnemonic, "movsx")) &&
            count >= 2) {
            // Determine source size
            RegSize src_size = RegSize::BYTE;
            if (ops[1].type == OperandType::Register || ops[1].type == OperandType::RegisterPlaceholder) {
                src_size = ops[1].reg_size;
            }
            else if (ops[1].type == OperandType::Memory || ops[1].type == OperandType::PointerPlaceholder) {
                src_size = ops[1].reg_size;
            }

            // Adjust opcode based on source size
            if (src_size == RegSize::WORD) {
                opcode2 += 1;  // 0xB6->0xB7 for movzx, 0xBE->0xBF for movsx
            }
        }

        buffer.emit(opcode2);
    }
    else {
        // MOV reg, imm has special encoding: opcode+reg instead of opcode+ModRM
        if (parser::str_equal(instr->mnemonic, "mov") && instr->type == EncodingType::REG_IMM) {
            uint8_t reg_code = uint8_t(resolve_register(ops[0])) & 0x7;

            if (size == RegSize::BYTE) {
                buffer.emit(0xB0 + reg_code);
            }
            else {
                buffer.emit(opcode + reg_code);
            }
        }
        else if (instr->type == EncodingType::JUMP_ABS || instr->type == EncodingType::CALL_ABS) {
            // Don't emit opcode here - it's emitted in the case statement below
        }
        else {
            buffer.emit(opcode);
        }
    }

    // === EMIT MODRM AND OPERANDS ===
    
    switch (instr->type) {
        case EncodingType::REG_REG:
            buffer.emit(0xC0 | (uint8_t(resolve_register(ops[1])) << 3) | uint8_t(resolve_register(ops[0])));
            break;
            
        case EncodingType::REG_MEM:
            if (ops[1].type == OperandType::PointerPlaceholder) {
                // For RIP-relative: only 4 bytes remain (the offset itself)
                emit_rip_relative_modrm(resolve_register(ops[0]), ops[1].placeholder_id, 4);
            }
            else {
                emit_memory_operand(resolve_register(ops[0]), ops[1].memory);
            }
            break;

        case EncodingType::MEM_REG:
            if (ops[0].type == OperandType::PointerPlaceholder) {
                // For RIP-relative: only 4 bytes remain (the offset itself)
                emit_rip_relative_modrm(resolve_register(ops[1]), ops[0].placeholder_id, 4);
            }
            else {
                emit_memory_operand(resolve_register(ops[1]), ops[0].memory);
            }
            break;

        case EncodingType::ACC_IMM:
            {
                uint32_t imm = uint32_t(ops[1].immediate);
                if (ops[1].type == OperandType::ImmediatePlaceholder && ops[1].placeholder_id < immediate_table.size())
                    imm = uint32_t(immediate_table[ops[1].placeholder_id]);
                
                if (size == RegSize::BYTE) {
                    buffer.emit(uint8_t(imm));
                } else if (size == RegSize::WORD) {
                    buffer.emit(uint8_t(imm));
                    buffer.emit(uint8_t(imm >> 8));
                } else {
                    buffer.emit_dword(imm);
                }
            }
            break;
            
        case EncodingType::REG_IMM:
            if (parser::str_equal(instr->mnemonic, "mov")) {
                // MOV reg, imm: immediate is encoded directly after opcode+reg (no ModRM)
                uint64_t imm = ops[1].immediate;
                if (ops[1].type == OperandType::ImmediatePlaceholder && ops[1].placeholder_id < immediate_table.size())
                    imm = immediate_table[ops[1].placeholder_id];
                
                if (size == RegSize::QWORD) {
                    buffer.emit_qword(imm);
                } else if (size == RegSize::DWORD) {
                    buffer.emit_dword(uint32_t(imm));
                } else if (size == RegSize::WORD) {
                    buffer.emit(uint8_t(imm));
                    buffer.emit(uint8_t(imm >> 8));
                } else {
                    buffer.emit(uint8_t(imm));
                }
            } else {
                // Other instructions: opcode + ModRM + immediate
                buffer.emit(0xC0 | (instr->reg_field << 3) | uint8_t(resolve_register(ops[0])));
                
                uint32_t imm = uint32_t(ops[1].immediate);
                if (ops[1].type == OperandType::ImmediatePlaceholder && ops[1].placeholder_id < immediate_table.size())
                    imm = uint32_t(immediate_table[ops[1].placeholder_id]);
                
                if (size == RegSize::BYTE) {
                    buffer.emit(uint8_t(imm));
                } else if (size == RegSize::WORD) {
                    buffer.emit(uint8_t(imm));
                    buffer.emit(uint8_t(imm >> 8));
                } else {
                    buffer.emit_dword(imm);
                }
            }
            break;
            
        case EncodingType::REG_IMM8:
            buffer.emit(0xC0 | (instr->reg_field << 3) | uint8_t(resolve_register(ops[0])));
            buffer.emit(uint8_t(ops[1].immediate & 0xFF));
            break;
            
        case EncodingType::SINGLE_REG:
            if (!parser::str_equal(instr->mnemonic, "push") && !parser::str_equal(instr->mnemonic, "pop"))
                buffer.emit(0xC0 | (instr->reg_field << 3) | uint8_t(resolve_register(ops[0])));
            break;
            
        case EncodingType::MEM_IMM:
        {
            // Calculate bytes after ModRM: offset (4) + immediate size
            size_t imm_size = (size == RegSize::BYTE) ? 1 :
                (size == RegSize::WORD) ? 2 : 4;
            size_t bytes_after_modrm = 4 + imm_size;  // RIP offset + immediate

            if (ops[0].type == OperandType::PointerPlaceholder)
                emit_rip_relative_modrm(static_cast<Reg>(instr->reg_field), ops[0].placeholder_id, bytes_after_modrm);
            else
                emit_memory_operand(static_cast<Reg>(instr->reg_field), ops[0].memory);

            uint64_t imm = ops[1].immediate;
            if (ops[1].type == OperandType::ImmediatePlaceholder && ops[1].placeholder_id < immediate_table.size())
                imm = immediate_table[ops[1].placeholder_id];

            if (size == RegSize::BYTE) {
                buffer.emit(uint8_t(imm));
            }
            else if (size == RegSize::WORD) {
                buffer.emit(uint8_t(imm));
                buffer.emit(uint8_t(imm >> 8));
            }
            else {
                buffer.emit_dword(uint32_t(imm));
            }
        }
        break;

        case EncodingType::MEM_IMM8:
        {
            // Calculate bytes after ModRM: offset (4) + immediate (1)
            size_t bytes_after_modrm = 4 + 1;

            if (ops[0].type == OperandType::PointerPlaceholder)
                emit_rip_relative_modrm(static_cast<Reg>(instr->reg_field), ops[0].placeholder_id, bytes_after_modrm);
            else
                emit_memory_operand(static_cast<Reg>(instr->reg_field), ops[0].memory);

            buffer.emit(uint8_t(ops[1].immediate & 0xFF));
        }
        break;

        case EncodingType::SINGLE_MEM:
            if (ops[0].type == OperandType::PointerPlaceholder) {
                // For RIP-relative: only 4 bytes remain (the offset itself)
                emit_rip_relative_modrm(static_cast<Reg>(instr->reg_field), ops[0].placeholder_id, 4);
            }
            else {
                emit_memory_operand(static_cast<Reg>(instr->reg_field), ops[0].memory);
            }
            break;

        case EncodingType::REG_CL:
            buffer.emit(0xC0 | (instr->reg_field << 3) | uint8_t(resolve_register(ops[0])));
            break;

        case EncodingType::MEM_CL:
            if (ops[0].type == OperandType::PointerPlaceholder) {
                emit_rip_relative_modrm(static_cast<Reg>(instr->reg_field), ops[0].placeholder_id, 4);
            }
            else {
                emit_memory_operand(static_cast<Reg>(instr->reg_field), ops[0].memory);
            }
            break;

        case EncodingType::REG_REG_IMM:
            buffer.emit(0xC0 | (uint8_t(resolve_register(ops[0])) << 3) | uint8_t(resolve_register(ops[1])));
            {
                uint32_t imm = uint32_t(ops[2].immediate);
                if (ops[2].type == OperandType::ImmediatePlaceholder && ops[2].placeholder_id < immediate_table.size())
                    imm = uint32_t(immediate_table[ops[2].placeholder_id]);
                buffer.emit_dword(imm);
            }
            break;

        case EncodingType::REG_REG_IMM8:
            buffer.emit(0xC0 | (uint8_t(resolve_register(ops[0])) << 3) | uint8_t(resolve_register(ops[1])));
            buffer.emit(uint8_t(ops[2].immediate & 0xFF));
            break;

        case EncodingType::REG_MEM_IMM:
            if (ops[1].type == OperandType::PointerPlaceholder) {
                size_t bytes_after_modrm = 4 + 4;  // offset + imm32
                emit_rip_relative_modrm(resolve_register(ops[0]), ops[1].placeholder_id, bytes_after_modrm);
            }
            else {
                emit_memory_operand(resolve_register(ops[0]), ops[1].memory);
            }
            {
                uint32_t imm = uint32_t(ops[2].immediate);
                if (ops[2].type == OperandType::ImmediatePlaceholder && ops[2].placeholder_id < immediate_table.size())
                    imm = uint32_t(immediate_table[ops[2].placeholder_id]);
                buffer.emit_dword(imm);
            }
            break;

        case EncodingType::REG_MEM_IMM8:
            if (ops[1].type == OperandType::PointerPlaceholder) {
                size_t bytes_after_modrm = 4 + 1;  // offset + imm8
                emit_rip_relative_modrm(resolve_register(ops[0]), ops[1].placeholder_id, bytes_after_modrm);
            }
            else {
                emit_memory_operand(resolve_register(ops[0]), ops[1].memory);
            }
            buffer.emit(uint8_t(ops[2].immediate & 0xFF));
            break;

        case EncodingType::JUMP_PTR:
        case EncodingType::CALL_PTR:
            if (ops[0].type == OperandType::PointerPlaceholder) {
                // RIP-relative indirect: FF /4 or FF /2 + disp32
                emit_rip_relative_modrm(static_cast<Reg>(instr->reg_field), ops[0].placeholder_id, 4);
            }
            else if (ops[0].type == OperandType::Memory) {
                emit_memory_operand(static_cast<Reg>(instr->reg_field), ops[0].memory);
            }
            else if (ops[0].type == OperandType::Register || ops[0].type == OperandType::RegisterPlaceholder) {
                // Direct register indirect: FF /4 or FF /2 + ModRM
                buffer.emit(0xC0 | (instr->reg_field << 3) | uint8_t(resolve_register(ops[0])));
            }
            break;
        case EncodingType::JUMP_ABS:
        case EncodingType::CALL_ABS:
            if (ops[0].type == OperandType::PointerPlaceholder && ops[0].placeholder_id < pointer_table.size()) {
                int32_t offset;
                if (calculate_absolute_offset(pointer_table[ops[0].placeholder_id], 5, offset)) {
                    buffer.emit(instr->opcode);  // E8 or E9
                    buffer.emit_dword(uint32_t(offset));
                }
                else {
                    // Error already printed, emit zeros
                    buffer.emit(instr->opcode);
                    buffer.emit_dword(0);
                }
            }
            break;

        default:
            break;
    }
}

void x64Assembler::emit_rip_relative_modrm(Reg reg_field, uint8_t ptr_id, size_t bytes_after_modrm) {
    buffer.emit(0x05 | (uint8_t(reg_field) << 3));
    if (ptr_id < pointer_table.size()) {
        int32_t offset;
        // bytes_after_modrm includes the 4-byte offset we're about to emit plus any following bytes
        if (!calculate_rip_offset(pointer_table[ptr_id], bytes_after_modrm, offset)) {
            buffer.emit_dword(0);
            return;
        }
        buffer.emit_dword(uint32_t(offset));
    }
    else {
        buffer.emit_dword(0);
    }
}

void x64Assembler::emit_memory_operand(Reg reg_field, const MemoryOperand& mem) {
    uint8_t modrm = uint8_t(reg_field) << 3;
    bool needs_sib = mem.has_index || (mem.has_base && mem.base == Reg::RSP);
    
    if (needs_sib) {
        if (mem.has_displacement) {
            if (mem.displacement >= -128 && mem.displacement <= 127) {
                buffer.emit(0x44 | modrm);
                buffer.emit(encode_sib(mem));
                buffer.emit(uint8_t(mem.displacement));
            } else {
                buffer.emit(0x84 | modrm);
                buffer.emit(encode_sib(mem));
                buffer.emit_dword(uint32_t(mem.displacement));
            }
        } else {
            buffer.emit(0x04 | modrm);
            buffer.emit(encode_sib(mem));
        }
    } else if (mem.has_base) {
        if (mem.has_displacement) {
            if (mem.displacement >= -128 && mem.displacement <= 127) {
                buffer.emit(0x40 | modrm | uint8_t(mem.base));
                buffer.emit(uint8_t(mem.displacement));
            } else {
                buffer.emit(0x80 | modrm | uint8_t(mem.base));
                buffer.emit_dword(uint32_t(mem.displacement));
            }
        } else {
            buffer.emit(modrm | uint8_t(mem.base));
        }
    }
}

uint8_t x64Assembler::encode_sib(const MemoryOperand& mem) const {
    uint8_t sib = 0;
    
    switch (mem.scale) {
        case 1: sib |= 0x00; break;
        case 2: sib |= 0x40; break;
        case 4: sib |= 0x80; break;
        case 8: sib |= 0xC0; break;
        default: sib |= 0x00; break;
    }
    
    if (mem.has_index) 
        sib |= (uint8_t(mem.index) & 0x7) << 3;
    else 
        sib |= 0x20;
    
    if (mem.has_base) 
        sib |= uint8_t(mem.base) & 0x7;
    else 
        sib |= 0x05;
    
    return sib;
}

} // namespace x64asm