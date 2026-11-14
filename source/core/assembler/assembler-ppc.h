#pragma once

// Minimal PowerPC (32-bit, big-endian) assembler

#include "dobby/dobby_internal.h"

#include "core/assembler/assembler.h"

#include "MemoryAllocator/CodeMemBuffer.h"

namespace zz {
namespace ppc {

struct Register {
  int code_;
  constexpr explicit Register(int code) : code_(code) {
  }
  constexpr int code() const {
    return code_;
  }
  constexpr bool Is(const Register &other) const {
    return code_ == other.code_;
  }
};

#define PPC_DEFINE_GPR(n) constexpr Register r##n(n);
PPC_DEFINE_GPR(0) PPC_DEFINE_GPR(1) PPC_DEFINE_GPR(2) PPC_DEFINE_GPR(3) PPC_DEFINE_GPR(4) PPC_DEFINE_GPR(5)
PPC_DEFINE_GPR(6) PPC_DEFINE_GPR(7) PPC_DEFINE_GPR(8) PPC_DEFINE_GPR(9) PPC_DEFINE_GPR(10) PPC_DEFINE_GPR(11)
PPC_DEFINE_GPR(12) PPC_DEFINE_GPR(13) PPC_DEFINE_GPR(14) PPC_DEFINE_GPR(15) PPC_DEFINE_GPR(16) PPC_DEFINE_GPR(17)
PPC_DEFINE_GPR(18) PPC_DEFINE_GPR(19) PPC_DEFINE_GPR(20) PPC_DEFINE_GPR(21) PPC_DEFINE_GPR(22) PPC_DEFINE_GPR(23)
PPC_DEFINE_GPR(24) PPC_DEFINE_GPR(25) PPC_DEFINE_GPR(26) PPC_DEFINE_GPR(27) PPC_DEFINE_GPR(28) PPC_DEFINE_GPR(29)
PPC_DEFINE_GPR(30) PPC_DEFINE_GPR(31)
#undef PPC_DEFINE_GPR

// r12 is volatile and never has an argument in the 32-bit SysV / EABI ABIs.
constexpr Register TMP_REG_0(PPC_TMP_REG_NDX_0);

// BO field values for bc
enum : int {
  BO_ALWAYS = 20,  // 0b10100
  BO_IF_TRUE = 12, // 0b01100
  BO_IF_FALSE = 4, // 0b00100
  BO_DNZ = 16,     // 0b10000
  BO_DZ = 18,      // 0b10010
};

struct Assembler : AssemblerBase {
  Assembler(addr_t fixed_addr) : AssemblerBase(fixed_addr) {
  }

  ~Assembler() {
  }

  void Emit(uint32_t value) {
    code_buffer_.Emit<uint32_t>(value);
  }

  void nop() {
    Emit(0x60000000); // ori r0, r0, 0
  }

  // integer

  void addi(Register rd, Register ra, int16_t simm) {
    Emit((14u << 26) | (rd.code() << 21) | (ra.code() << 16) | (uint16_t)simm);
  }

  // addis rd, 0, imm
  void lis(Register rd, uint16_t imm) {
    Emit((15u << 26) | (rd.code() << 21) | imm);
  }

  void ori(Register ra, Register rs, uint16_t imm) {
    Emit((24u << 26) | (rs.code() << 21) | (ra.code() << 16) | imm);
  }

  // load / store

  void lwz(Register rt, Register ra, int16_t d) {
    Emit((32u << 26) | (rt.code() << 21) | (ra.code() << 16) | (uint16_t)d);
  }

  void stw(Register rs, Register ra, int16_t d) {
    Emit((36u << 26) | (rs.code() << 21) | (ra.code() << 16) | (uint16_t)d);
  }

  void stwu(Register rs, Register ra, int16_t d) {
    Emit((37u << 26) | (rs.code() << 21) | (ra.code() << 16) | (uint16_t)d);
  }

  // special registers

  void mflr(Register rd) {
    Emit(0x7C0802A6u | (rd.code() << 21));
  }
  void mtlr(Register rs) {
    Emit(0x7C0803A6u | (rs.code() << 21));
  }
  void mfctr(Register rd) {
    Emit(0x7C0902A6u | (rd.code() << 21));
  }
  void mtctr(Register rs) {
    Emit(0x7C0903A6u | (rs.code() << 21));
  }

  // branch

  // relative, offset is in bytes, must be 4-aligned and within +-32MB
  void b(int32_t offset, bool lk = false) {
    Emit((18u << 26) | ((uint32_t)offset & 0x03FFFFFCu) | (lk ? 1u : 0u));
  }

  // relative conditional, offset is in bytes, must be 4-aligned and within +-32KB
  void bc(int bo, int bi, int16_t offset, bool lk = false) {
    Emit((16u << 26) | ((uint32_t)bo << 21) | ((uint32_t)bi << 16) | ((uint16_t)offset & 0xFFFCu) | (lk ? 1u : 0u));
  }

  void blr() {
    Emit(0x4E800020u);
  }
  void bctr() {
    Emit(0x4E800420u);
  }
  void bctrl() {
    Emit(0x4E800421u);
  }
};

struct TurboAssembler : Assembler {
public:
  TurboAssembler() : TurboAssembler(0) {
  }

  TurboAssembler(addr_t fixed_addr) : Assembler(fixed_addr) {
  }

  ~TurboAssembler() {
  }

  // lis + ori
  void Mov32(Register rd, uint32_t imm) {
    lis(rd, (uint16_t)(imm >> 16));
    ori(rd, rd, (uint16_t)(imm & 0xFFFF));
  }

  // lis, ori, mtctr, bctr. Clobbers TMP_REG_0 and CTR.
  void JumpAbsolute(uint32_t address) {
    Mov32(TMP_REG_0, address);
    mtctr(TMP_REG_0);
    bctr();
  }

  // lis, ori, mtctr, bctrl. Clobbers TMP_REG_0, CTR and LR.
  void CallAbsolute(uint32_t address) {
    Mov32(TMP_REG_0, address);
    mtctr(TMP_REG_0);
    bctrl();
  }

  void CallFunction(ExternalReference function) {
    CallAbsolute((uint32_t)(uintptr_t)function.address);
  }
};

} // namespace ppc
} // namespace zz

// The PPC assembler uses no pseudo labels with pending fixups, but AssemblerBase::bindLabel references this.
inline void PseudoLabel::link_confused_instructions(CodeMemBuffer *buffer) {
  ref_insts.clear();
}
