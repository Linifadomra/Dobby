#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_PPC)

#include "dobby/dobby_internal.h"

#include "core/assembler/assembler-ppc.h"

#include "InterceptRouting/RoutingPlugin.h"

using namespace zz::ppc;

// 16 bytes: lis r12, hi(to) ; ori r12, r12, lo(to) ; mtctr r12 ; bctr
// Clobbers r12 and CTR, both volatile in the 32-bit SysV / EABI ABIs.
Trampoline *GenerateNormalTrampolineBuffer(addr_t from, addr_t to) {
  __FUNC_CALL_TRACE__();
  TurboAssembler turbo_assembler_(from);
#undef _
#define _ turbo_assembler_. // NOLINT: clang-tidy

  _ JumpAbsolute((uint32_t)to);
  DEBUG_LOG("[trampoline] use [lis, ori, mtctr, bctr]");

  auto tramp_buffer = turbo_assembler_.code_buffer();
  auto tramp_block = tramp_buffer->dup();
  auto tramp = new Trampoline(TRAMPOLINE_PPC_LIS_ORI_BCTR, tramp_block);
  DEBUG_LOG("[trampoline] trampoline addr: %p(temp), %p(real), size: %d", tramp->addr(), from, tramp->size());
  debug_hex_log_buffer((uint8_t *)tramp->addr(), tramp->size());
  return tramp;
}

#endif
