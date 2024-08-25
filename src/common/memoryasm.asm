
; NeOS: A simple 64-bit operating system
; Copyright (C) 2024 Joel Marti

; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.

; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
; GNU General Public License for more details.

; You should have received a copy of the GNU General Public License
; along with this program. If not, see <https://www.gnu.org/licenses/>.

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
