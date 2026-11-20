#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_IA32)

#include "dobby/dobby_internal.h"

#include "core/assembler/assembler-ia32.h"

#include "TrampolineBridge/ClosureTrampolineBridge/ClosureTrampoline.h"
#include "TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.h"

using namespace zz;
using namespace zz::x86;

ClosureTrampoline *GenerateClosureTrampoline(void *carry_data, void *carry_handler) {
  auto closure_tramp = new ClosureTrampoline(CLOSURE_TRAMPOLINE_X86, CodeMemBlock{}, carry_data, carry_handler);

  if (!closure_bridge_addr) {
    closure_bridge_init();
  }

  const int tramp_size = 32;
  auto blk = gMemoryAllocator.allocExecBlock(tramp_size);
  if (blk.addr() == 0) {
    delete closure_tramp;
    return nullptr;
  }

#define _ turbo_assembler_.
  TurboAssembler turbo_assembler_(blk.addr());

  // sub esp, 4 (6 bytes) + mov [esp], imm32 (7 bytes) + jmp rel32 (5 bytes)
  const int jmp_end_off = 6 + 7 + 5;
  int32_t offset = (int32_t)((addr_t)closure_bridge_addr - (blk.addr() + jmp_end_off));

  _ sub(esp, Immediate(4, 32));
  _ mov(Address(esp, 0), Immediate((int32_t)(uintptr_t)closure_tramp, 32));
  _ jmp(Immediate(offset, 32));

  auto tramp_block = AssemblerCodeBuilder::FinalizeFromTurboAssembler(&turbo_assembler_);
  closure_tramp->buffer = tramp_block;
  return closure_tramp;
}

#endif
