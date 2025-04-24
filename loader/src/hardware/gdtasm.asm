
%define NUM_SEGMENTS 6

    extern g_gdt

    global WriteGDT
WriteGDT:
    mov word [gdtr.nLimit], NUM_SEGMENTS * 16 - 1
    mov qword [gdtr.nBase], g_gdt
    lgdt [gdtr]
    push 0x10 ; Kernel Code Segment
    lea rax, [rel .ReloadCS]
    push rax
    retfq
.ReloadCS:
    mov ax, 0x20 ; Kernel Data Segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

    global FlushTSS
FlushTSS:
    push ax
    mov ax, 20
    ltr ax
    pop ax
    ret

gdtr:
.nLimit dw 0
.nBase  dq 0
