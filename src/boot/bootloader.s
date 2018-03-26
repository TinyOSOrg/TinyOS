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
times 128 dq 0x0

; 总内存容量
; 跟据ndisasm的结果，bootloader被加载后memory_byte_size位于0xd23处
memory_byte_size dd 0x512

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

; 用来临时存放ARDS结构体的区域
; 不知道会有多少，区域留个20个吧
tmp_ARDS_buffer times 400 db 0x0
; ARDS结构体数量
tmp_ARDS_count dw 0x0

;-----------------------------------------------------
; bootloader

bootloader_start:

    ; 用int15h获取内存总容量

    mov ebx, 0 ; 初始化ARDS Continuation
    mov edx, 0x534d4150
    mov di, tmp_ARDS_buffer
    
int15h_e820_loop_start:

    mov eax, 0xe820
    mov ecx, 20
    int 0x15

    inc word [tmp_ARDS_count]
    add di, cx

    cmp ebx, 0
    jnz int15h_e820_loop_start ; ebx为0表示已经得到全部ARDS结构体

    ; 找到ARDS中最大的那个，作为总容量
    mov cx, [tmp_ARDS_count]

    mov edx, 0
    mov ebx, tmp_ARDS_buffer

find_max_ards_size:

    mov eax, [ebx]
    add eax, [ebx + 8]
    add ebx, 20

    cmp edx, eax
    jg next_ards

    mov edx, eax

next_ards:

    loop find_max_ards_size

    mov [memory_byte_size], edx

    ; Hello, protection mode!

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
