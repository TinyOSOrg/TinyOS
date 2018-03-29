%include "src/boot/boot.s"

SEGMENT_SELECTOR_VIDEO equ (0x3 << 3) + \
                           SEGMENT_SELECTOR_ATTRIB_USE_GDT + \
                           SEGMENT_SELECTOR_ATTRIB_RPL_0

[bits 32]
section .text

global _reset_cursor
global _put_char

_reset_cursor:
    pushad
    mov ax, SEGMENT_SELECTOR_VIDEO
    mov gs, ax

    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    mov al, 0
    out dx, al
    
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, 0
    out dx, al

    popad
    ret

_put_char:
    pushad
    mov ax, SEGMENT_SELECTOR_VIDEO
    mov gs, ax

    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    in al, dx
    mov ah, al

    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in al, dx

    mov bx, ax
    mov ecx, [esp + 36]

    cmp cl, 0xd
    jz is_cr ; CR
    cmp cl, 0xa
    jz is_lf ; LF
    cmp cl, 0x8
    jz is_bs ; backspace
    jmp is_normal_char
    
is_bs:
    dec bx
    shl bx, 1
    
    mov byte [gs:bx], 0x20
    inc bx
    mov byte [gs:bx], 0x07
    shr bx, 1
    jmp update_cursor

is_normal_char:
    shl bx, 1

    mov [gs:bx], cl
    inc bx
    mov byte [gs:bx], 0x07
    shr bx, 1
    inc bx
    cmp bx, 2000
    jl update_cursor

is_lf:
is_cr:
    mov dx, 0
    mov ax, bx
    mov si, 80

    div si

    sub bx, dx

    add bx, 80
    cmp bx, 2000

    jl update_cursor

roll_scr:
    cld
    mov eax, 960
    
    mov esi, 0xc00b80a0
    mov edi, 0xc00b8000

    rep movsd

    mov ebx, 3840
    mov ecx, 80

cls:
    mov word [gs:ebx], 0x0720
    add ebx, 2
    loop cls
    mov bx, 1920

update_cursor:
    mov dx, 0x03d4
    mov al, 0x0e
    out dx, al
    mov dx, 0x03d5
    mov al, bh
    out dx, al
    
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, bl
    out dx, al

    popad
    ret
