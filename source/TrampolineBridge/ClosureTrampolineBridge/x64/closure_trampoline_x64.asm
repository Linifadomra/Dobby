PUBLIC closure_trampoline_asm
PUBLIC closure_trampoline_asm_end
PUBLIC closure_tramp_entry_addr
PUBLIC closure_bridge_addr

.code

closure_trampoline_asm PROC

    push QWORD PTR closure_tramp_entry_addr

    jmp  QWORD PTR closure_bridge_addr


closure_trampoline_asm ENDP

.data

ALIGN 8

closure_tramp_entry_addr dq 0
closure_bridge_addr dq 0

.code

closure_trampoline_asm_end PROC
    ret
closure_trampoline_asm_end ENDP


END












