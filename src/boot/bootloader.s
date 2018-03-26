%include "src/boot/boot.s"

SECTION BOOT_LOADER vstart=BOOTLOADER_START_ADDR
    jmp bootloader_start

;-----------------------------------------------------
; GDT数据

GDT_START:
    dd 0x0
    dd 0x0
GDT_CODE_DESC:
    dd 0xffff
    dd SEGMENT_DESC_CODE_HIGH
GDT_DATA_DESC:
    dd 0xffff
    dd SEGMENT_DESC_DATA_HIGH
GDT_VIDEO_DESC:
    dd 0x80000007
    dd SEGMENT_DESC_VIDEO_HIGH

GDT_BYTE_SIZE equ $ - GDT_START

; 预留一些GDT空位
times 50 dq 0x0

; 段选择子
SEGMENT_SELECTOR_CODE  equ (0x1 << 3) + \
                           SEGMENT_SELECTOR_ATTRIB_RPL_0 + \
                           SEGMENT_SELECTOR_ATTRIB_USE_GDT
SEGMENT_SELECTOR_DATA  equ (0x2 << 3) + \
                           SEGMENT_SELECTOR_ATTRIB_RPL_0 + \
                           SEGMENT_SELECTOR_ATTRIB_USE_GDT
SEGMENT_SELECTOR_VIDEO equ (0x3 << 3) + \
                           SEGMENT_SELECTOR_ATTRIB_RPL_0 + \
                           SEGMENT_SELECTOR_ATTRIB_USE_GDT

GDT_ENTRY dw GDT_BYTE_SIZE - 1
          dd GDT_START

;-----------------------------------------------------
; bootloader
; Hello, protection mode!

bootloader_start:

    ; 打开A20地址线
    in al, 0x92
    or al, 0x2
    out 0x92, al

    ; 设置GDT
    lgdt [GDT_ENTRY]

    ; 保护模式开关
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; 清空流水线
    jmp dword SEGMENT_SELECTOR_CODE:new_world_in_protection_mode

[bits 32]
new_world_in_protection_mode:

;-----------------------------------------------------
; 设置段选择子

    mov ax, SEGMENT_SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov ax, SEGMENT_SELECTOR_VIDEO
    mov gs, ax

    mov esp, BOOTLOADER_START_ADDR

;-----------------------------------------------------

    mov byte [gs:0], 'A'

    jmp $
