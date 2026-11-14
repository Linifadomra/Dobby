#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_PPC)

#include "dobby/dobby_internal.h"

#include "core/assembler/assembler-ppc.h"

#include "InterceptRouting/RoutingPlugin.h"

using namespace zz::ppc;

// I-form `b` reaches +-32MB
#define PPC_B_XXX_RANGE (1ll << 25)

// Single `b dst`. If dst is out of range this returns nullptr and the caller falls back to the 16 byte normal trampoline
Trampoline *GenerateNearTrampolineBuffer(addr_t src, addr_t dst) {
  __FUNC_CALL_TRACE__();
  DEBUG_LOG("near trampoline: %p -> %p", src, dst);

  int64_t distance = (int64_t)(int32_t)dst - (int64_t)(int32_t)src;
  if (distance >= PPC_B_XXX_RANGE || distance < -PPC_B_XXX_RANGE) {
    DEBUG_LOG("[near trampoline] target out of `b` range, fallback to normal trampoline");
    return nullptr;
  }

  TurboAssembler turbo_assembler_(src);
#undef _
#define _ turbo_assembler_. // NOLINT: clang-tidy

  _ b((int32_t)distance);

  auto tramp_buffer = turbo_assembler_.code_buffer();
  auto tramp_block = tramp_buffer->dup();
  auto tramp = new Trampoline(TRAMPOLINE_PPC_B_XXX, tramp_block);
  DEBUG_LOG("[near trampoline] trampoline addr: %p(temp), %p(real), size: %d", tramp->addr(), src, tramp->size());
  debug_hex_log_buffer((uint8_t *)tramp->addr(), tramp->size());
  return tramp;
}

#endif
