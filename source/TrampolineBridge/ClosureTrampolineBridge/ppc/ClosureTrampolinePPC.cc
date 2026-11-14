#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_PPC)

#include "dobby/dobby_internal.h"
#include "core/assembler/assembler-ppc.h"
#include "TrampolineBridge/ClosureTrampolineBridge/ClosureTrampoline.h"
#include "TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.h"

using namespace zz;
using namespace zz::ppc;

#define TRAMP_FRAME 32

ClosureTrampoline *GenerateClosureTrampoline(void *carry_data, void *carry_handler) {
  auto closure_tramp = new ClosureTrampoline(CLOSURE_TRAMPOLINE_PPC, CodeMemBlock{}, carry_data, carry_handler);

  if (!closure_bridge_addr) {
    closure_bridge_init();
  }

  TurboAssembler turbo_assembler_;
#undef _
#define _ turbo_assembler_. // NOLINT: clang-tidy

  // r12 and CTR are already clobbered by the hook site trampoline, r0 and LR are saved here.
  _ stwu(r1, r1, -TRAMP_FRAME);
  _ stw(r0, r1, 8);
  _ mflr(r0);
  _ stw(r0, r1, 12);

  _ Mov32(TMP_REG_0, (uint32_t)(uintptr_t)closure_tramp);
  _ stw(TMP_REG_0, r1, 16);

  _ CallAbsolute((uint32_t)(uintptr_t)closure_bridge_addr);

  // back from the bridge: r12 = next hop, LR / r0 were written back into our frame
  _ lwz(r0, r1, 12);
  _ mtlr(r0);
  _ lwz(r0, r1, 8);
  _ addi(r1, r1, TRAMP_FRAME);

  _ mtctr(TMP_REG_0);
  _ bctr();

  auto tramp_block = AssemblerCodeBuilder::FinalizeFromTurboAssembler(&turbo_assembler_);
  closure_tramp->buffer = tramp_block;

  DEBUG_LOG("closure trampoline addr: %p, size: %d", closure_tramp->addr(), closure_tramp->size());
  debug_hex_log_buffer((uint8_t *)closure_tramp->addr(), closure_tramp->size());
  return closure_tramp;
}

#endif
