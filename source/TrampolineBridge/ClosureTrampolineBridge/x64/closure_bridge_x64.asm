PUBLIC closure_bridge_asm
PUBLIC closure_bridge_asm_end
PUBLIC common_closure_bridge_handler_addr

EXTERN common_closure_bridge_handler:PROC

.code

closure_bridge_asm PROC

    ; flags register
    pushfq

    ; used for alignment
    sub rsp, 8

    ; general register
    sub rsp, 16*8
    mov [rsp+8*0], rax
    mov [rsp+8*1], rbx
    mov [rsp+8*2], rcx
    mov [rsp+8*3], rdx
    mov [rsp+8*4], rbp
    mov [rsp+8*5], rsp
    mov [rsp+8*6], rdi
    mov [rsp+8*7], rsi
    mov [rsp+8*8], r8
    mov [rsp+8*9], r9
    mov [rsp+8*10], r10
    mov [rsp+8*11], r11
    mov [rsp+8*12], r12
    mov [rsp+8*13], r13
    mov [rsp+8*14], r14
    mov [rsp+8*15], r15

    ; rsp_offset = 8*5
    ; orig_rsp_offset = 16*8+2*8+8 = 144
    mov rax, rsp
    add rax, 144
    ; rsp+rsp_offset
    mov [rsp+40], rax

    ; closure_tramp_entry_offset = 16*8+2*8 = 144-8 = 136
    mov rdi, rsp
    mov rsi, [rsp+136]

    mov rax, rsp
    and rax, 0Fh
    jz aligned_call

    push rax
    call common_closure_bridge_handler
    pop rax
    jmp call_end

aligned_call:
    call common_closure_bridge_handler

call_end:

    ; restore registers (reverse order)
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rbp
    add rsp, 8
    pop rdi
    pop rsi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    add rsp, 8
    popfq

    ret

closure_bridge_asm ENDP

closure_bridge_asm_end PROC
    ret
closure_bridge_asm_end ENDP

.data

ALIGN 8

common_closure_bridge_handler_addr QWORD common_closure_bridge_handler

END