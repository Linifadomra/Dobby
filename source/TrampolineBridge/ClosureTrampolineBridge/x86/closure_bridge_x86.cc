#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_IA32)

#include "dobby/dobby_internal.h"

#include "core/assembler/assembler-ia32.h"

#include "TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.h"

using namespace zz;
using namespace zz::x86;

void closure_bridge_init() {
#define _ turbo_assembler_.
#define __ turbo_assembler_.code_buffer()->

  const uint8_t pushfd = 0x9c;
  const uint8_t popfd = 0x9d;

  TurboAssembler turbo_assembler_(0);

  // general register
  _ sub(esp, Immediate(8 * 4, 32));
  _ mov(Address(esp, 4 * 0), eax);
  _ mov(Address(esp, 4 * 1), ebx);
  _ mov(Address(esp, 4 * 2), ecx);
  _ mov(Address(esp, 4 * 3), edx);
  _ mov(Address(esp, 4 * 4), ebp);
  _ mov(Address(esp, 4 * 5), esp);
  _ mov(Address(esp, 4 * 6), edi);
  _ mov(Address(esp, 4 * 7), esi);

  // save flags register
  __ Emit<uint8_t>(pushfd);
  _ pop(eax);
  _ sub(esp, Immediate(2 * 4, 32));
  _ mov(Address(esp, 4), eax);

  // save origin sp
  _ mov(eax, esp);
  _ add(eax, Immediate(8 * 4 + 2 * 4 + 4, 32));
  _ sub(esp, Immediate(2 * 4, 32));
  _ mov(Address(esp, 4), eax);

  // handler(ctx, tramp), cdecl, 16-byte aligned stack
  _ mov(eax, esp);
  _ mov(ecx, Address(esp, 2 * 4 + 2 * 4 + 8 * 4));
  _ mov(ebp, esp);
  _ sub(esp, Immediate(16, 32));
  // and esp, 0xFFFFFFF0
  __ Emit<uint8_t>(0x83);
  __ Emit<uint8_t>(0xE4);
  __ Emit<uint8_t>(0xF0);
  _ mov(Address(esp, 0), eax);
  _ mov(Address(esp, 4), ecx);

  _ CallFunction(ExternalReference((void *)common_closure_bridge_handler));

  _ mov(esp, ebp);

  // skip dummy/sp placeholder, dummy_1
  _ add(esp, Immediate(2 * 4 + 4, 32));
  __ Emit<uint8_t>(popfd);

  _ pop(eax);
  _ pop(ebx);
  _ pop(ecx);
  _ pop(edx);
  _ pop(ebp);
  _ add(esp, Immediate(4, 32));
  _ pop(edi);
  _ pop(esi);

  // the carry_data slot left by the closure trampoline is the return address
  _ ret();

  _ relocDataLabels();

  auto closure_bridge = AssemblerCodeBuilder::FinalizeFromTurboAssembler(&turbo_assembler_);
  closure_bridge_addr = (asm_func_t)closure_bridge.addr();
}

#endif
