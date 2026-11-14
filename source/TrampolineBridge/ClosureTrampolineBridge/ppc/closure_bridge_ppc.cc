#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_PPC)

#include "dobby/dobby_internal.h"

#include "core/assembler/assembler-ppc.h"

#include "TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.h"

#include <stddef.h>

// closure_bridge_ppc.S hardcodes the context layout
static_assert(offsetof(DobbyRegisterContext, sp) == 4, "ctx layout mismatch");
static_assert(offsetof(DobbyRegisterContext, lr) == 8, "ctx layout mismatch");
static_assert(offsetof(DobbyRegisterContext, ctr) == 12, "ctx layout mismatch");
static_assert(offsetof(DobbyRegisterContext, cr) == 16, "ctx layout mismatch");
static_assert(offsetof(DobbyRegisterContext, xer) == 20, "ctx layout mismatch");
static_assert(offsetof(DobbyRegisterContext, general) == 24, "ctx layout mismatch");
static_assert(offsetof(DobbyRegisterContext, floating) == 152, "ctx layout mismatch");

extern "C" void closure_bridge_asm();

void closure_bridge_init() {
  __FUNC_CALL_TRACE__();
  closure_bridge_addr = (asm_func_t)closure_bridge_asm;
  DEBUG_LOG("[closure bridge] closure bridge at %p", closure_bridge_addr);
}

void set_routing_bridge_next_hop(DobbyRegisterContext *ctx, void *address) {
  ctx->general.regs.r12 = (uint32_t)(uintptr_t)address;
}

void get_routing_bridge_next_hop(DobbyRegisterContext *ctx, void *address) {
}

#endif
