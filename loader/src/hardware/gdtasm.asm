
    default rel

%define NUM_SEGMENTS 7 ; One extra since the TSS descriptor is 16 bytes.

    section .text

    global WriteGDT
WriteGDT:
    push rax

    mov word [gdtr.nLimit], NUM_SEGMENTS * 8 - 1
    lea rax, [rel g_sGDT]
    mov qword [gdtr.nBase], rax
    lgdt [gdtr]
    push 0x08 ; Kernel Code Segment
    lea rax, [rel .ReloadCS]
    push rax
    retfq
.ReloadCS:
    mov ax, 0x10 ; Kernel Data Segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    pop rax
    ret

    global FlushTSS
FlushTSS:
    push ax
    mov ax, 0x28 ; Task State Segment
    ltr ax
    pop ax
    ret

    section .data
    extern g_sGDT

gdtr:
.nLimit dw 0
.nBase  dq 0
