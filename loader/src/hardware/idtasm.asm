
    section .text

%macro IRQ 1
    global HandleInterrupt%1
HandleInterrupt%1:
    cli
    mov byte [nInterrupt], %1
    push qword 0
    jmp Interrupt
%endmacro

%macro EXC_ERR 1
    global HandleException%1
HandleException%1:
    cli
    mov byte [nInterrupt], %1
    jmp Interrupt
%endmacro

%macro EXC_NOERR 1
    global HandleException%1
HandleException%1:
    cli
    mov byte [nInterrupt], %1
    push qword 0
    jmp Interrupt
%endmacro


EXC_NOERR 0x00
EXC_NOERR 0x01
EXC_NOERR 0x02
EXC_NOERR 0x03
EXC_NOERR 0x04
EXC_NOERR 0x05
EXC_NOERR 0x06
EXC_NOERR 0x07
EXC_ERR   0x08
EXC_NOERR 0x09
EXC_ERR   0x0A
EXC_ERR   0x0B
EXC_ERR   0x0C
EXC_ERR   0x0D
EXC_ERR   0x0E
EXC_NOERR 0x0F
EXC_NOERR 0x10
EXC_ERR   0x11
EXC_NOERR 0x12
EXC_NOERR 0x13
EXC_NOERR 0x14
EXC_NOERR 0x15
EXC_NOERR 0x16
EXC_NOERR 0x17
EXC_NOERR 0x18
EXC_NOERR 0x19
EXC_NOERR 0x1A
EXC_NOERR 0x1B
EXC_NOERR 0x1C
EXC_NOERR 0x1D
EXC_ERR   0x1E
EXC_NOERR 0x1F

IRQ 0x20
IRQ 0x21
IRQ 0x22
IRQ 0x23
IRQ 0x81

Interrupt:

    cli ; Disable interrupts so only one can happen at once
    ; Push all registers (pusha doesn't exist in x64)
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

    ; The compiler uses registers instead of the stack, for some reason.
    mov rdx, qword [rsp]         ; nErrorCode
    mov rsi, rsp                 ; qwRSP
    movzx rdi, byte [nInterrupt] ; nInterrupt

    extern HandleInterrupt
    call HandleInterrupt
    mov rsp, rax ; Set the stack to the returned value

    pop rax
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

    add rsp, 8
    
    sti ; Re-enable interrupts so more can come
    global IgnoreInterrupt
IgnoreInterrupt:
    iretq

    section .data
nInterrupt db 0
