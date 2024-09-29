
    global LoadGDT
LoadGDT:
    shl si, 4
    dec si
    mov [gdtr.nLimit], si
    mov [gdtr.nBase], rdi
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
