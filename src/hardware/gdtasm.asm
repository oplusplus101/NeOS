
%define NUM_SEGMENTS 5

    extern g_gdt

    global LoadGDT
LoadGDT:
    mov word [gdtr.nLimit], NUM_SEGMENTS * 16 - 1
    mov qword [gdtr.nBase], g_gdt
    lgdt [gdtr]
    push 0x10
    lea rax, [rel .ReloadCS]
    push rax
    retfq
.ReloadCS:
    mov ax, 0x20
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

gdtr:
.nLimit dw 0
.nBase  dq 0
