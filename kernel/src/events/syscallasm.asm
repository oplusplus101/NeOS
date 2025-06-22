
	section .text

    global SetupSysInstructions
SetupSysInstructions:
	mov rcx, 0xc0000082 ; LSTAR
	wrmsr
	mov rcx, 0xc0000080 ; EFER
	rdmsr
	or eax, 1
	wrmsr
	mov rcx, 0xc0000081 ; STAR
	rdmsr
	mov edx, 0x00180008 ; ???
	wrmsr
    ret
