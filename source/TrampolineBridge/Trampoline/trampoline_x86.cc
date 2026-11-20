#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_IA32)

#include "dobby/dobby_internal.h"

#include "core/assembler/assembler-ia32.h"
#include "core/codegen/codegen-ia32.h"

#include "InstructionRelocation/x86/InstructionRelocationX86.h"

#include "MemoryAllocator/NearMemoryAllocator.h"
#include "InterceptRouting/RoutingPlugin.h"

using namespace zz::x86;

Trampoline *GenerateNormalTrampolineBuffer(addr_t from, addr_t to) {
  TurboAssembler turbo_assembler_(from);

  CodeGen codegen(&turbo_assembler_);
  codegen.JmpNear((uint32_t)to);

  auto tramp_block = turbo_assembler_.code_buffer()->dup();
  return new Trampoline(TRAMPOLINE_X86_JMP, tramp_block);
}

Trampoline *GenerateNearTrampolineBuffer(addr_t src, addr_t dst) {
  return nullptr;
}

#endif
