#include "platform_detect_macro.h"

#if defined(TARGET_ARCH_PPC)

#include "dobby/dobby_internal.h"

#include "core/assembler/assembler-ppc.h"

using namespace zz::ppc;

// On PowerPC the only PC-relative instructions are the relative branches:
//   - I-form  `b / bl`   (primary opcode 18, AA = 0), +-32MB
//   - B-form  `bc / bcl` (primary opcode 16, AA = 0), +-32KB
// `blr / bctr / bclr / bcctr` are register based and absolute branches (AA = 1) need no fixup

static inline int32_t sign_extend(uint32_t value, int bits) {
  uint32_t m = 1u << (bits - 1);
  return (int32_t)((value ^ m) - m);
}

static inline bool inst_is_b_bl(uint32_t inst) {
  return (inst >> 26) == 18;
}

static inline bool inst_is_bc(uint32_t inst) {
  return (inst >> 26) == 16;
}

struct relo_ctx_t {
  addr_t cursor;

  CodeMemBlock *origin;
  CodeMemBlock relocated{};
  CodeMemBuffer *relocated_buffer;

  explicit relo_ctx_t(MemBlock *origin) : origin(origin) {
    cursor = origin->addr();
  }

  uint32_t preferred_relo_size() {
    return origin->size;
  }

  void correct_final_relo_size() {
    origin->resize(relo_size());
  }

  uint32_t relo_size() {
    return (uintptr_t)cursor - origin->addr();
  }

  addr_t origin_start() {
    return origin->addr();
  }

  addr_t origin_cursor() {
    return origin->addr() + relo_size();
  }

  bool in_patched_range(addr_t addr) {
    return addr >= origin_start() && addr < origin_start() + preferred_relo_size();
  }

  int relocate(bool branch);
};

int relo_ctx_t::relocate(bool branch) {
  TurboAssembler turbo_assembler_;
#undef _
#define _ turbo_assembler_. // NOLINT

  this->relocated_buffer = turbo_assembler_.code_buffer();

  int idx = 0;
  while (relo_size() < preferred_relo_size()) {
    uint32_t inst = *(uint32_t *)origin_cursor();
    addr_t pc = origin_cursor();

    if (inst_is_b_bl(inst) || inst_is_bc(inst)) {
      bool is_b_form = inst_is_bc(inst);
      bool aa = (inst >> 1) & 1;
      bool lk = inst & 1;

      if (aa) {
        // absolute branch, target does not depend on pc
        DEBUG_LOG("%d:relo <branch absolute> at %p", idx, pc);
        _ Emit(inst);
      } else {
        int32_t offset = is_b_form ? sign_extend(inst & 0xFFFC, 16) : sign_extend(inst & 0x03FFFFFC, 26);
        addr_t dst = pc + offset;
        int bo = is_b_form ? ((inst >> 21) & 0x1F) : BO_ALWAYS;
        bool always = (bo & 0x14) == 0x14;

        if (in_patched_range(dst) && dst != origin_start()) {
          // the destination bytes are overwritten by the trampoline, there is nothing sane to branch to
          ERROR_LOG("relo: branch at %p targets %p inside the patched range", pc, dst);
          return -1;
        }

        if (always) {
          if (lk && dst == pc + 4) {
            // `bcl 20, 31, $+4` / `bl $+4`: the get-pc idiom, LR must hold the *original* address of the next insn
            DEBUG_LOG("%d:relo <get_pc> at %p", idx, pc);
            _ Mov32(TMP_REG_0, (uint32_t)(pc + 4));
            _ mtlr(TMP_REG_0);
          } else {
            DEBUG_LOG("%d:relo <b_bl> at %p -> %p", idx, pc, dst);
            _ Mov32(TMP_REG_0, (uint32_t)dst);
            _ mtctr(TMP_REG_0);
            if (lk)
              _ bctrl();
            else
              _ bctr();
          }
        } else {
          if (lk) {
            // LR would have to be written on the not-taken path as well, and there is no scratch register left
            ERROR_LOG("relo: conditional branch-and-link at %p is not supported", pc);
            return -1;
          }
          DEBUG_LOG("%d:relo <bc> at %p -> %p", idx, pc, dst);
          _ Emit((inst & 0xFFFF0000u) | 8u);
          _ b(4 * 5);
          _ JumpAbsolute((uint32_t)dst);
        }
      }
    } else {
      _ Emit(inst);
    }

    idx++;
    this->cursor += sizeof(uint32_t);
  }

  correct_final_relo_size();

  // TODO: if last instr is an unconditional branch, this is dead code
  if (branch) {
    _ JumpAbsolute((uint32_t)origin_cursor());
  }

  relocated = AssemblerCodeBuilder::FinalizeFromTurboAssembler(&turbo_assembler_);
  return 0;
}

void GenRelocateCode(void *buffer, CodeMemBlock *origin, CodeMemBlock *relocated, bool branch) {
  relo_ctx_t ctx(origin);
  if (ctx.relocate(branch) != 0) {
    *relocated = CodeMemBlock{};
    return;
  }
  *relocated = ctx.relocated;
}

void GenRelocateCodeAndBranch(void *buffer, CodeMemBlock *origin, CodeMemBlock *relocated) {
  GenRelocateCode(buffer, origin, relocated, true);
}

#endif
