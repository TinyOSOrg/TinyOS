SECTION BOOT_LOADER vstart=0x900
    ; 字符输出所使用的段基址
    mov ax, 0xb800
    mov gs, ax

    mov byte [gs:0x00], 'H'
    mov byte [gs:0x01], 0x04

    mov byte [gs:0x02], 'e'
    mov byte [gs:0x03], 0x04

    mov byte [gs:0x04], 'l'
    mov byte [gs:0x05], 0x04

    mov byte [gs:0x06], 'l'
    mov byte [gs:0x07], 0x04

    mov byte [gs:0x08], 'o'
    mov byte [gs:0x09], 0x04

    mov byte [gs:0x0a], ','
    mov byte [gs:0x0b], 0x04

    mov byte [gs:0x0c], 'O'
    mov byte [gs:0x0d], 0x04

    mov byte [gs:0x0e], 'S'
    mov byte [gs:0x0f], 0x04

    mov byte [gs:0x10], '!'
    mov byte [gs:0x11], 0x04

    jmp $
    