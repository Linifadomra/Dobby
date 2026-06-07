#include "platform_detect_macro.h"

#if defined(TARGET_ARCH_X64)

#include "dobby/dobby_internal.h"

#include "InstructionRelocation/x64/InstructionRelocationX64.h"
#include "InstructionRelocation/x86/x86_insn_decode/x86_insn_decode.h"

#include "core/arch/x64/registers-x64.h"
#include "core/assembler/assembler-x64.h"
#include "core/codegen/codegen-x64.h"

#include "MemoryAllocator/CodeMemBuffer.h"

using namespace zz::x64;

struct PrefixState {
    bool has_rex = false;
    int length = 0;
};

static int GetModRMExtraLength(uint8_t modrm) {
    const uint8_t mod = (modrm >> 6) & 0x3;
    const uint8_t rm  = modrm & 0x7;

    int extra_length = 0;

    // ModRM displacement:
    //  mod = 00 -> no displacement
    //  mod = 01 -> disp8
    //  mod = 10 -> disp32
    //  mod = 11 -> register-direct addressing
    if (mod == 1)
        extra_length += 1;
    else if (mod == 2)
        extra_length += 4;

    // rm = 100 with memory addressing indicates a SIB byte follows.
    if (mod != 3 && rm == 4)
        extra_length += 1;

    return extra_length;
}

static inline bool IsByteInRange(uint8_t v, uint8_t lo, uint8_t hi) {
    return v >= lo && v <= hi;
}

// workaround for dobby's decoder :/
static int GetRealInstructionLength(const uint8_t* code) {
    const uint8_t* start = code;

    while (true) {
        uint8_t c = *code;

        // REX
        if (IsByteInRange(c,0x40,0x4F)) {
            code++;
            continue;
        }

        switch (c) {
            case 0xF0:
            case 0xF2:
            case 0xF3:
            case 0x2E:
            case 0x36:
            case 0x3E:
            case 0x26:
            case 0x64:
            case 0x65:
            case 0x66:
                code++;
                continue;
        }

        break;
    }

    uint8_t op = *code++;
    int len = (int)(code - start);

    auto consume_modrm = [&]() {
        uint8_t modrm = *code++;
        len++;

        len += GetModRMExtraLength(modrm);
    };
    
    // PUSH/POP r64
    if (IsByteInRange(op,0x50,0x5F))
        return len + 1;

    if (IsByteInRange(op,0xB8,0xBF))
        return len + 4; 

    switch (op) {
        // MOV r, r/m
        case 0x89:
        case 0x8B:
            consume_modrm();
            return len;

        // SSE / two-byte opcodes
        case 0x0F: {
            uint8_t op2 = *code++;
            len++;

            switch (op2) {

                case 0x10:
                case 0x11:
                case 0x28:
                case 0x29:
                case 0x2E:
                case 0x2F:
                case 0x58:
                case 0x59:
                case 0x5C:
                case 0x5E:
                    consume_modrm();
                    return len;

                default:
                    consume_modrm();
                    return len;
            }
        }


        default:
            break;
    }

    return len + 2;
}

int GenRelocateCodeFixed(void *buffer, CodeMemBlock *origin, CodeMemBlock *relocated, bool branch) {
  TurboAssembler turbo_assembler_(0);
  // Set fixed executable code chunk address
  turbo_assembler_.set_fixed_addr(relocated->addr());
#define ASM turbo_assembler_.
#define BUF turbo_assembler_.code_buffer()->

  auto curr_orig_ip = (addr64_t)origin->addr();
  auto curr_relo_ip = (addr64_t)relocated->addr();

  auto buffer_cursor = (uint8_t *)buffer;

  int actual_copied_size = 0;
  const int min_hook_size = 14; 

  while (actual_copied_size < min_hook_size) {
    x86_insn_decode_t insn = {0};
    memset(&insn, 0, sizeof(insn));
    int real_length = GetRealInstructionLength(buffer_cursor);
    int size_before = turbo_assembler_.code_buffer()->buffer_size;

    GenRelocateSingleX86Insn(curr_orig_ip, curr_relo_ip, buffer_cursor, &turbo_assembler_,
                             turbo_assembler_.code_buffer(), insn, 64);

    int size_after = turbo_assembler_.code_buffer()->buffer_size;
    if ((size_after - size_before) != real_length) {
      turbo_assembler_.code_buffer()->buffer_size = size_before;
      turbo_assembler_.code_buffer()->EmitBuffer(buffer_cursor, real_length);
    }

    insn.length = real_length;
    actual_copied_size += insn.length;

    curr_orig_ip += insn.length;
    buffer_cursor += insn.length;
    curr_relo_ip = (addr64_t)relocated->addr() + turbo_assembler_.pc_offset();
  }

  // jmp to the origin rest instructions
  if (branch) {
    CodeGen codegen(&turbo_assembler_);
    // TODO: 6 == jmp [RIP + disp32] instruction size
    addr64_t stub_addr = curr_relo_ip + 6;
    codegen.JmpNearIndirect(stub_addr);
    turbo_assembler_.code_buffer()->Emit<int64_t>(curr_orig_ip);
  }

  // update origin
  auto new_origin_len = curr_orig_ip - origin->addr();
  origin->reset(origin->addr(), new_origin_len);

  int relo_len = turbo_assembler_.code_buffer()->buffer_size;
  if (relo_len > relocated->size) {
    DEBUG_LOG("pre-alloc code chunk not enough");
    return -1;
  }

  auto relocated_ = AssemblerCodeBuilder::FinalizeFromTurboAssembler(&turbo_assembler_);
  *relocated = relocated_;

  return 0;
}

void GenRelocateCodeAndBranch(void *buffer, CodeMemBlock *origin, CodeMemBlock *relocated) {
  GenRelocateCode(buffer, origin, relocated, true);
}

void GenRelocateCode(void *buffer, CodeMemBlock *origin, CodeMemBlock *relocated, bool branch) {
  GenRelocateCodeX86Shared(buffer, origin, relocated, branch);
}

#endif
