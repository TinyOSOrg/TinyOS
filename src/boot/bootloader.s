%include "src/boot/boot.s"

section BOOT_LOADER vstart=BOOTLOADER_START_ADDR

    jmp bootloader_start ; jmp near占3字节，故GDT开头在BOOTLOADER_START_ADDR + 3

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

GDT_BYTE_SIZE equ $ - GDT_START

; 预留一些GDT空位
times 128 dq 0x0

; kernel.bin重定位后的入口地址
kernel_entry dd 0x0

; 段选择子
SEGMENT_SELECTOR_CODE  equ (0x1 << 3) + \
                           SEGMENT_SELECTOR_ATTRIB_RPL_0 + \
                           SEGMENT_SELECTOR_ATTRIB_USE_GDT
SEGMENT_SELECTOR_DATA  equ (0x2 << 3) + \
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

    mov [TOTAL_MEMORY_SIZE_ADDR], edx

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
    mov gs, ax

    mov esp, BOOTLOADER_START_ADDR

;-----------------------------------------------------
; 加载内核文件

    mov eax, KERNEL_START_SECTOR
    mov ebx, KERNEL_START_ADDR
    mov ecx, KERNEL_SECTOR_COUNT

    call read_disk_under_protection_mode

;-----------------------------------------------------
; 分页

    call init_page

    ; 存下描述符表地址
    sgdt [GDT_ENTRY]

    ; 修改video段基址，使之使用虚拟地址
    mov ebx, [GDT_ENTRY + 0x2]
    or dword [ebx + 0x1c], 0xc0000000

    ; 修改描述符表地址，使之使用虚拟地址
    add dword [GDT_ENTRY + 0x2], 0xc0000000

    ; 使栈指针使用虚拟地址
    add esp, 0xc0000000

    ; 启用分页

    mov eax, PAGE_DIR_ENTRY_ADDR
    mov cr3, eax

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ; 重新加载描述符表
    lgdt [GDT_ENTRY]

;-----------------------------------------------------
; 进入内核

    jmp SEGMENT_SELECTOR_CODE:hello_kernel

hello_kernel:

    call init_kernel
    mov esp, 0xc01ff000
    jmp [kernel_entry]

;-----------------------------------------------------
; 启用分页
; PDE放在0x200000处，占据4096字节，其上是PTE
; PDE + 0x1000就是第一个PTE入口

;  通过虚拟地址访问：
;       页目录所在页的首字节地址：0xfffff000
;       页目录的第0xuvw项：     0xfffffuvw
;       10位二进制值M作为页表索引，12位页表内偏移N，则页表中的项为：(0x3ff << 22) | (M << 12) | N

init_page:

    ; 清空PDE和前255个页表
    mov esi, 0x0
    mov ecx, 0x400 * (1 + 255)
clear_PDE:
    mov dword [PAGE_DIR_ENTRY_ADDR + esi * 4], 0x0
    inc esi
    loop clear_PDE

    ; 创建PDE

    mov eax, (PAGE_DIR_ENTRY_ADDR + 0x1000) | PAGE_USER_USER | PAGE_READ_WRITE_READ_WRITE | PAGE_PRESENT_TRUE
    mov [PAGE_DIR_ENTRY_ADDR + 0xc00], eax  ; 页目录第768项指向首个页表
    mov [PAGE_DIR_ENTRY_ADDR], eax          ; 页目录第0项指向首个页表

    ; 页目录最后一项指向页目录本身所在页面
    ; 也就是说以后可以通过访问虚拟地址顶端的4KB来访问页目录

    sub eax, 0x1000
    mov [PAGE_DIR_ENTRY_ADDR + 0xffc], eax

    ; 把首个页表的前1024项指向物理地址的低4M空间
    ; 保证开启分页后bootloader还能用

    mov ebx, PAGE_DIR_ENTRY_ADDR + 0x1000
    mov ecx, 0x400
    mov esi, 0x0
    mov edx, PAGE_USER_USER | PAGE_READ_WRITE_READ_WRITE | PAGE_PRESENT_TRUE
make_PTE:
    mov [ebx + esi * 4], edx
    add edx, 0x1000
    inc esi
    loop make_PTE

    ; 把页目录第769项指向第1个页表，以此类推直到页目录的第1022项
    ; 虚拟空间中3GB～4GB是操作系统专用，存放在0～255页表中

    mov eax, (PAGE_DIR_ENTRY_ADDR + 0x2000) | PAGE_USER_USER | PAGE_READ_WRITE_READ_WRITE | PAGE_PRESENT_TRUE
    mov ebx, PAGE_DIR_ENTRY_ADDR
    mov ecx, 0xfe
    mov esi, 0x301
make_kernel_PDE:
    mov [ebx + esi * 4], eax
    inc esi
    add eax, 0x1000
    loop make_kernel_PDE

    ret

;-----------------------------------------------------
; kernel重定位

init_kernel:
    
    mov eax, 0x0
    mov ecx, 0x0
    mov edx, 0x0

    ; 取得kernel入口

    mov eax, [KERNEL_START_ADDR + 0x18]
    mov [kernel_entry], eax

    ; 取得segments信息

    mov dx, [KERNEL_START_ADDR + 0x2a]  ; 取得program header大小
    mov ebx, [KERNEL_START_ADDR + 0x1c] ; 取得第一个ph偏移量
    add ebx, KERNEL_START_ADDR          ; 计算出第一个ph地址
    mov cx, [KERNEL_START_ADDR + 0x2c]  ; 取得ph数量

for_each_seg:

    cmp byte [ebx], 0x0
    je ph_unused

    push dword [ebx + 0x10]
    mov eax, [ebx + 0x4]
    add eax, KERNEL_START_ADDR
    push eax
    push dword [ebx + 0x8]
    call memcpy
    add esp, 0xc

ph_unused:

    add ebx, edx
    loop for_each_seg

    ret

;-----------------------------------------------------
; 保护模式下读硬盘，抄的实模式下的那个
; eax: LBA
; ebx: addr
; cx: sector count

read_disk_under_protection_mode:

    mov esi, eax
    mov di, cx
    
    mov dx, 0x1f2
    mov al, cl
    out dx, al

    mov eax, esi

    mov dx, 0x1f3                       
    out dx, al                          

    mov cl, 0x8
    shr eax, cl
    mov dx, 0x1f4
    out dx, al

    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    shr eax, cl
    and al, 0x0f
    or al, 0xe0
    mov dx, 0x1f6
    out dx, al

    mov dx, 0x1f7
    mov al, 0x20                        
    out dx, al

waiting_disk_reading:
    nop
    in al, dx
    and al, 0x88
    cmp al, 0x08
    jnz waiting_disk_reading

    mov ax, di

    mov dx, 0x100
    mul dx
    mov cx, ax	   
    mov dx, 0x1f0
read_from_disk_port_to_mem:
    in ax, dx		
    mov [ebx], ax
    add ebx, 0x2

    loop read_from_disk_port_to_mem

    ret

;-----------------------------------------------------
; memcpy
; 和c的memcpy(dst, src, size)约定一样

memcpy:

    cld
    
    push ebp
    mov ebp, esp
    push ecx

    mov edi, [ebp + 0x8]
    mov esi, [ebp + 0xc]
    mov ecx, [ebp + 0x10]

    rep movsb

    pop ecx
    pop ebp

    ret
