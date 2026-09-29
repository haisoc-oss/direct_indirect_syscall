.code
DirectSyscall PROC
    push    rbx             ;store value of rbx 
    mov     rbx , rsp       ;save current stack pointer before alloacte more space for new stack
    sub     rsp , 40        ;allocate space for new stack

    xor     rax, rax        ;clear out value store in rax
    movzx   eax, cx         ;mov SSN store in rcx to rax

    cmp     rdx, 4          ;Check if api function have 4 or more agrument
    jle     Four_agru

    
    sub     rdx,4           ;sub to find many agrument need to push in stack
    imul    rdx,8           ;mul to caculate size of stack frame for agrument
    sub     rsp,rdx         ;allocate space on stack for agrument
    mov     [rbx+16], rdx   ;save rdx value

    More_agru:
    mov     r11, [rbx+56+rdx]       ;this loop will begin from the last argrument to the fifth argrument
    mov     r11, [r11]
    mov     [rsp + 32 + rdx], r11
    sub     rdx, 8
    cmp     rdx,0
    jne     More_agru

    Four_agru:
    mov     rcx, [r8] 
    mov     rdx, [r9]
    mov     r11, [rbx + 48]
    mov     r8, [r11]
    mov     r11, [rbx + 56]
    mov     r9, [r11]

    mov     r10, rcx
    syscall

    mov rbx, [rbx+16]
    add rsp, rbx
    add rsp, 40
    pop rbx
    ret

DirectSyscall ENDP
END
