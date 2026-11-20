#include "platform_detect_macro.h"

#if defined(TARGET_ARCH_IA32)

#include "dobby/dobby_internal.h"

#include "InstructionRelocation/x86/InstructionRelocationX86.h"
#include "InstructionRelocation/x86/x86_insn_decode/x86_insn_decode.h"

#include "core/arch/x86/registers-x86.h"
#include "core/assembler/assembler-ia32.h"
#include "core/codegen/codegen-ia32.h"

#include "MemoryAllocator/CodeMemBuffer.h"

using namespace zz::x86;

int GenRelocateCodeFixed(void *buffer, CodeMemBlock *origin, CodeMemBlock *relocated, bool branch) {
  TurboAssembler turbo_assembler_(0);
  turbo_assembler_.set_fixed_addr(relocated->addr());

  auto curr_orig_ip = (addr32_t)origin->addr();
  auto curr_relo_ip = (addr32_t)relocated->addr();

  uint8_t *buffer_cursor = (uint8_t *)buffer;

  int predefined_relocate_size = origin->size;

  while ((buffer_cursor < ((uint8_t *)buffer + predefined_relocate_size))) {
    x86_insn_decode_t insn = {0};
    memset(&insn, 0, sizeof(insn));
    GenRelocateSingleX86Insn(curr_orig_ip, curr_relo_ip, buffer_cursor, &turbo_assembler_,
                             turbo_assembler_.code_buffer(), insn, 32);

    curr_orig_ip += insn.length;
    buffer_cursor += insn.length;
    curr_relo_ip = (addr32_t)relocated->addr() + turbo_assembler_.pc_offset();
  }

  if (branch) {
    CodeGen codegen(&turbo_assembler_);
    codegen.JmpNear(curr_orig_ip);
  }

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
