.section .text
.global boot
.extern main
.extern Pip_VCLI
.global apboot
.extern ap_main
boot:
    push %ebx
    call Pip_VCLI
    call main

loop:
    jmp loop

/* APs entrypoint */
apboot:
    push %ebx
    push %eax
    call Pip_VCLI
    call ap_main
    jmp loop

