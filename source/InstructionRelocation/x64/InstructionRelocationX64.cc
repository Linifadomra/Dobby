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

int GenRelocateCodeFixed(void *buffer, CodeMemBlock *origin, CodeMemBlock *relocated, bool branch) {
  TurboAssembler turbo_assembler_(0);
  turbo_assembler_.set_fixed_addr(relocated->addr());
#define _ turbo_assembler_.
#define __ turbo_assembler_.code_buffer()->

  auto curr_orig_ip = (addr64_t)origin->addr();
  auto curr_relo_ip = (addr64_t)relocated->addr();
  auto buffer_cursor = (uint8_t *)buffer;

  // The size requested by the trampoline (e.g., 6 bytes)
  int minimum_required_size = origin->size; 
  int total_bytes_decoded = 0;

  // CRITICAL FIX: Loop until we satisfy the minimum size requirement.
  // Do not allow loose pointer comparisons to overshoot into partial instructions.
  while (total_bytes_decoded < minimum_required_size) {
    x86_insn_decode_t insn = {0};
    memset(&insn, 0, sizeof(insn));
    
    GenRelocateSingleX86Insn(curr_orig_ip, curr_relo_ip, buffer_cursor, &turbo_assembler_,
                             turbo_assembler_.code_buffer(), insn, 64);

    if (insn.length == 0) {
      DEBUG_LOG("Decoder failure on instruction at %p", (void*)curr_orig_ip);
      return -1;
    }

    // Advance iteration state safely 
    curr_orig_ip       += insn.length;
    buffer_cursor      += insn.length;
    total_bytes_decoded += insn.length;
    curr_relo_ip        = (addr64_t)relocated->addr() + turbo_assembler_.pc_offset();
  }

  // jmp back to the remaining instructions
  if (branch) {
    CodeGen codegen(&turbo_assembler_);
    addr64_t stub_addr = curr_relo_ip + 6;
    codegen.JmpNearIndirect(stub_addr);
    turbo_assembler_.code_buffer()->Emit<int64_t>(curr_orig_ip);
  }

  // Correct the origin length structure back to the precise instruction boundary (e.g., 9)
  origin->reset(origin->addr(), total_bytes_decoded);

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