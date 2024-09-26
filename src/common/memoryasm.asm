
    global _memset
_memset:
    pop rdi     ; Destination address
    pop rax     ; Value
    pop rcx     ; Number of bytes
    rep stosb
    ret

    global _memcpy
_memcpy:
    pop rdi     ; Destination address
    pop rsi     ; Source address
    pop rcx     ; Number of bytes
    rep movsb
    ret

    global _strlen
_strlen:
    pop rsi
    xor rbx, rbx
.countloop:
    lodsb
    test al, al
    jz .done
    inc rbx
    jmp .countloop
.done:
    mov rax, rbx
    ret

    global _memzero
_memzero:
    pop rdi
    pop rcx
    xor al, al
    rep stosb
    ret
    