#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_IA32)

#include "core/assembler/assembler-ia32.h"

using namespace zz::x86;

void Assembler::jmp(Immediate imm) {
  code_buffer_.Emit<int8_t>(0xE9);
  code_buffer_.Emit<int32_t>((int)imm.value());
}

#endif
