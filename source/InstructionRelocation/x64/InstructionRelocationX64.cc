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

// workaround for dobby's decoder :/
static int GetRealInstructionLength(const uint8_t* code) {
    switch (code[0]) {
        case 0x55: // push rbp
        case 0x57: // push rdi
        case 0x53: // push rbx
            return 1;

        case 0x89:
        case 0x8B:
            return 2 + GetModRMExtraLength(code[1]);

        case 0x48:
            switch (code[1]) {
                case 0x89:
                case 0x8B:
                    return 3 + GetModRMExtraLength(code[2]);

                case 0x83:
                    return 4 + GetModRMExtraLength(code[2]);

                default:
                    break;
            }
            break;

        default:
            break;
    }

    return 1;
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
