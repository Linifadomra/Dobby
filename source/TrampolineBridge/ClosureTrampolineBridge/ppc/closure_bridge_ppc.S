#if defined(__powerpc__) || defined(__PPC__) || defined(__ppc__)
#if !defined(__powerpc64__) && !defined(__PPC64__)

#if defined(FULL_FLOATING_POINT_REGISTER_PACK)
#define FPR_COUNT 32
#elif defined(DOBBY_PPC_NO_FPU)
#define FPR_COUNT 0
#else
#define FPR_COUNT 14
#endif

#define CTX_OFF 16
#define CTX_SIZE (152 + 8 * FPR_COUNT)
#define FRAME (((CTX_OFF + CTX_SIZE) + 15) & ~15)

#define CTX_SP (CTX_OFF + 4)
#define CTX_LR (CTX_OFF + 8)
#define CTX_CTR (CTX_OFF + 12)
#define CTX_CR (CTX_OFF + 16)
#define CTX_XER (CTX_OFF + 20)
#define CTX_GPR (CTX_OFF + 24)
#define CTX_FPR (CTX_OFF + 152)

#define TRAMP_FRAME 32

.text
.align 2

.globl closure_bridge_asm
closure_bridge_asm:
  stwu 1, -FRAME(1)
  mflr 0
  stw 0, (FRAME + 4)(1)

  // save r0 - r31 (r0 currently holds LR, fixed up below)
  stmw 0, CTX_GPR(1)

  lwz 4, (FRAME + 8)(1)
  stw 4, (CTX_GPR + 0)(1)          // original r0
  lwz 4, (FRAME + 12)(1)
  stw 4, CTX_LR(1)                 // original lr
  addi 4, 1, (FRAME + TRAMP_FRAME)
  stw 4, CTX_SP(1)                 // original sp
  stw 4, (CTX_GPR + 4)(1)          // original r1
  mfctr 4
  stw 4, CTX_CTR(1)
  mfcr 4
  stw 4, CTX_CR(1)
  mfxer 4
  stw 4, CTX_XER(1)

#if FPR_COUNT >= 14
  stfd 0, (CTX_FPR + 0 * 8)(1)
  stfd 1, (CTX_FPR + 1 * 8)(1)
  stfd 2, (CTX_FPR + 2 * 8)(1)
  stfd 3, (CTX_FPR + 3 * 8)(1)
  stfd 4, (CTX_FPR + 4 * 8)(1)
  stfd 5, (CTX_FPR + 5 * 8)(1)
  stfd 6, (CTX_FPR + 6 * 8)(1)
  stfd 7, (CTX_FPR + 7 * 8)(1)
  stfd 8, (CTX_FPR + 8 * 8)(1)
  stfd 9, (CTX_FPR + 9 * 8)(1)
  stfd 10, (CTX_FPR + 10 * 8)(1)
  stfd 11, (CTX_FPR + 11 * 8)(1)
  stfd 12, (CTX_FPR + 12 * 8)(1)
  stfd 13, (CTX_FPR + 13 * 8)(1)
#endif
#if FPR_COUNT == 32
  stfd 14, (CTX_FPR + 14 * 8)(1)
  stfd 15, (CTX_FPR + 15 * 8)(1)
  stfd 16, (CTX_FPR + 16 * 8)(1)
  stfd 17, (CTX_FPR + 17 * 8)(1)
  stfd 18, (CTX_FPR + 18 * 8)(1)
  stfd 19, (CTX_FPR + 19 * 8)(1)
  stfd 20, (CTX_FPR + 20 * 8)(1)
  stfd 21, (CTX_FPR + 21 * 8)(1)
  stfd 22, (CTX_FPR + 22 * 8)(1)
  stfd 23, (CTX_FPR + 23 * 8)(1)
  stfd 24, (CTX_FPR + 24 * 8)(1)
  stfd 25, (CTX_FPR + 25 * 8)(1)
  stfd 26, (CTX_FPR + 26 * 8)(1)
  stfd 27, (CTX_FPR + 27 * 8)(1)
  stfd 28, (CTX_FPR + 28 * 8)(1)
  stfd 29, (CTX_FPR + 29 * 8)(1)
  stfd 30, (CTX_FPR + 30 * 8)(1)
  stfd 31, (CTX_FPR + 31 * 8)(1)
#endif

  // common_closure_bridge_handler(ctx, closure_trampoline)
  addi 3, 1, CTX_OFF
  lwz 4, (FRAME + 16)(1)
  bl common_closure_bridge_handler

#if FPR_COUNT >= 14
  lfd 0, (CTX_FPR + 0 * 8)(1)
  lfd 1, (CTX_FPR + 1 * 8)(1)
  lfd 2, (CTX_FPR + 2 * 8)(1)
  lfd 3, (CTX_FPR + 3 * 8)(1)
  lfd 4, (CTX_FPR + 4 * 8)(1)
  lfd 5, (CTX_FPR + 5 * 8)(1)
  lfd 6, (CTX_FPR + 6 * 8)(1)
  lfd 7, (CTX_FPR + 7 * 8)(1)
  lfd 8, (CTX_FPR + 8 * 8)(1)
  lfd 9, (CTX_FPR + 9 * 8)(1)
  lfd 10, (CTX_FPR + 10 * 8)(1)
  lfd 11, (CTX_FPR + 11 * 8)(1)
  lfd 12, (CTX_FPR + 12 * 8)(1)
  lfd 13, (CTX_FPR + 13 * 8)(1)
#endif
#if FPR_COUNT == 32
  lfd 14, (CTX_FPR + 14 * 8)(1)
  lfd 15, (CTX_FPR + 15 * 8)(1)
  lfd 16, (CTX_FPR + 16 * 8)(1)
  lfd 17, (CTX_FPR + 17 * 8)(1)
  lfd 18, (CTX_FPR + 18 * 8)(1)
  lfd 19, (CTX_FPR + 19 * 8)(1)
  lfd 20, (CTX_FPR + 20 * 8)(1)
  lfd 21, (CTX_FPR + 21 * 8)(1)
  lfd 22, (CTX_FPR + 22 * 8)(1)
  lfd 23, (CTX_FPR + 23 * 8)(1)
  lfd 24, (CTX_FPR + 24 * 8)(1)
  lfd 25, (CTX_FPR + 25 * 8)(1)
  lfd 26, (CTX_FPR + 26 * 8)(1)
  lfd 27, (CTX_FPR + 27 * 8)(1)
  lfd 28, (CTX_FPR + 28 * 8)(1)
  lfd 29, (CTX_FPR + 29 * 8)(1)
  lfd 30, (CTX_FPR + 30 * 8)(1)
  lfd 31, (CTX_FPR + 31 * 8)(1)
#endif

  lwz 0, CTX_CR(1)
  mtcrf 0xff, 0
  lwz 0, CTX_XER(1)
  mtxer 0

  // hand ctx->lr and ctx->r0 back to the closure trampoline. it restores them after we return
  lwz 0, CTX_LR(1)
  stw 0, (FRAME + 12)(1)
  lwz 0, (CTX_GPR + 0)(1)
  stw 0, (FRAME + 8)(1)

  // restore r2 - r31, r12 is now the next hop
  lmw 2, (CTX_GPR + 2 * 4)(1)

  lwz 0, (FRAME + 4)(1)
  mtlr 0
  addi 1, 1, FRAME
  blr

.globl closure_bridge_asm_end
closure_bridge_asm_end:

#endif
#endif
