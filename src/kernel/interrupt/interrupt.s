[bits 32]

%define WITH_ERR_CODE push 0
%define WITHOUT_ERR_CODE nop

extern put_str
extern intr_function

%macro INTR_VECTOR 2
    section .intr_vec_text
    intr_%1_entry:
        ; 统一错误码所占栈空间
        %2

        push ds
        push es
        push fs
        push gs
        pushad

        push eax
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        pop eax

        mov al, 0x20
        out 0xa0, al
        out 0x20, al

        push dword %1 ; 中断号作为intr_function参数，不管对方用不用
        call [intr_function + %1 * 4]

        jmp intr_proc_end

    section .intr_vec_data
        dd intr_%1_entry
%endmacro

section .intr_vec_text
global intr_proc_end
intr_proc_end:
    add esp, 4 ; 弹出中断号和错误码参数
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4 ; 弹出错误码
    iret

section .intr_vec_data

global intr_entry_table
intr_entry_table:

INTR_VECTOR 0x00, WITH_ERR_CODE     ; 除0
INTR_VECTOR 0x01, WITH_ERR_CODE     ; debug
INTR_VECTOR 0x02, WITH_ERR_CODE     ; NMI
INTR_VECTOR 0x03, WITH_ERR_CODE     ; 断点
INTR_VECTOR 0x04, WITH_ERR_CODE     ; INTF
INTR_VECTOR 0x05, WITH_ERR_CODE     ; 越界
INTR_VECTOR 0x06, WITH_ERR_CODE     ; 未知指令
INTR_VECTOR 0x07, WITH_ERR_CODE     ; 设备不可用
INTR_VECTOR 0x08, WITHOUT_ERR_CODE  ; double fault
INTR_VECTOR 0x09, WITH_ERR_CODE     ; 无fpu
INTR_VECTOR 0x0a, WITHOUT_ERR_CODE  ; 非法TSS
INTR_VECTOR 0x0b, WITHOUT_ERR_CODE  ; seg not p
INTR_VECTOR 0x0c, WITH_ERR_CODE     ; 栈段GG
INTR_VECTOR 0x0d, WITHOUT_ERR_CODE  ; GP
INTR_VECTOR 0x0e, WITHOUT_ERR_CODE  ; pagefault
INTR_VECTOR 0x0f, WITH_ERR_CODE     ; reserved
INTR_VECTOR 0x10, WITH_ERR_CODE     ; 浮点错误
INTR_VECTOR 0x11, WITHOUT_ERR_CODE  ; alignment check
INTR_VECTOR 0x12, WITH_ERR_CODE     ; machine check
INTR_VECTOR 0x13, WITH_ERR_CODE     ; SIMD浮点异常
INTR_VECTOR 0x14, WITH_ERR_CODE     ; reserved
INTR_VECTOR 0x15, WITH_ERR_CODE     ; reserved
INTR_VECTOR 0x16, WITH_ERR_CODE     ; reserved
INTR_VECTOR 0x17, WITH_ERR_CODE     ; reserved
INTR_VECTOR 0x18, WITHOUT_ERR_CODE  ; reserved
INTR_VECTOR 0x19, WITH_ERR_CODE     ; reserved
INTR_VECTOR 0x1a, WITHOUT_ERR_CODE  ; reserved
INTR_VECTOR 0x1b, WITHOUT_ERR_CODE  ; reserved
INTR_VECTOR 0x1c, WITH_ERR_CODE     ; reserved
INTR_VECTOR 0x1d, WITHOUT_ERR_CODE  ; reserved
INTR_VECTOR 0x1e, WITHOUT_ERR_CODE  ; reserved
INTR_VECTOR 0x1f, WITH_ERR_CODE     ; reserved
INTR_VECTOR 0x20, WITH_ERR_CODE     ; 时钟
INTR_VECTOR 0x21, WITH_ERR_CODE     ; 键盘
INTR_VECTOR 0x22, WITH_ERR_CODE     ; 级联用
INTR_VECTOR 0x23, WITH_ERR_CODE     ; 串口2
INTR_VECTOR 0x24, WITH_ERR_CODE     ; 串口1
INTR_VECTOR 0x25, WITH_ERR_CODE     ; 并口2
INTR_VECTOR 0x26, WITH_ERR_CODE     ; 软盘
INTR_VECTOR 0x27, WITH_ERR_CODE     ; 并口1
INTR_VECTOR 0x28, WITH_ERR_CODE     ; 实时时钟
INTR_VECTOR 0x29, WITH_ERR_CODE     ; 重定向
INTR_VECTOR 0x2a, WITH_ERR_CODE     ; reserved
INTR_VECTOR 0x2b, WITH_ERR_CODE     ; reserved
INTR_VECTOR 0x2c, WITH_ERR_CODE     ; ps/2鼠标
INTR_VECTOR 0x2d, WITH_ERR_CODE     ; fpu异常
INTR_VECTOR 0x2e, WITH_ERR_CODE     ; 硬盘
INTR_VECTOR 0x2f, WITH_ERR_CODE     ; reserved

extern syscall_func_table

section .text

global global_syscall_entry
global_syscall_entry:

    push 0
    push ds
    push es
    push fs
    push gs
    pushad

    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    pop eax

    push 0x80

    push edx
    push ecx
    push ebx
    call [syscall_func_table + eax * 4]

    add esp, 12
    mov [esp + 4 * 8], eax
    jmp intr_proc_end
