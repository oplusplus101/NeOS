
    default rel

    section .text

    extern g_sTSS

    global HandleSyscallStub
HandleSyscallStub:
    swapgs ; Apparently necessary
    
    mov r12, rsp ; Keep the user's stack
    mov rsp, qword [rel g_sTSS + 4] ; sTaskStateSegment.qwRSP0

    ; Push all
    push rbp
    push rdi
    push rsi

    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8

    push rdx
    push rcx
    push rbx
    push rax

    ; Push all the arguments
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rdx
    push rbx
    push rax

    mov rdi, rsp ; Pass it as a struct
    mov rbp, rsp

    extern HandleSyscall
    call HandleSyscall

    add rsp, 64 ; Clean up the arguments

    ; Pop all
    add rsp, 8 ; Don't pop rax, as it contains the syscall's return value
    pop rbx
    pop rcx
    pop rdx

    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    pop rsi
    pop rdi
    pop rbp

    mov r12, rsp ; Restore the user's stack    

    swapgs
    sysret

