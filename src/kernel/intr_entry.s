[bits 32]

%define WITH_ERR_CODE push 0
%define WITHOUT_ERR_CODE nop

extern put_str
extern intr_function

section INTR_ENTRY

%macro INTR_VECTOR 2
    section INTR_VEC
    intr_%1_entry:
        ; 统一错误码所占栈空间
        %2

        push ds
        push es
        push fs
        push gs
        pushad
        
        mov al, 0x20
        out 0xa0, al
        out 0x20, al

        push %1 ; 中断号作为intr_function参数，不管对方用不用
        call [intr_function + %1 * 4]
    
        jmp intr_proc_end

    section INTR_ENTRY
        dd intr_%1_entry
%endmacro

section INTR_ENTRY
intr_proc_end:
    add esp, 4 ; 弹出中断号
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 4 ; 弹出错误码
    iret

global intr_entry_table
intr_entry_table:

INTR_VECTOR 0x00, WITH_ERR_CODE
INTR_VECTOR 0x01, WITH_ERR_CODE
INTR_VECTOR 0x02, WITH_ERR_CODE
INTR_VECTOR 0x03, WITH_ERR_CODE 
INTR_VECTOR 0x04, WITH_ERR_CODE
INTR_VECTOR 0x05, WITH_ERR_CODE
INTR_VECTOR 0x06, WITH_ERR_CODE
INTR_VECTOR 0x07, WITH_ERR_CODE 
INTR_VECTOR 0x08, WITHOUT_ERR_CODE
INTR_VECTOR 0x09, WITH_ERR_CODE
INTR_VECTOR 0x0a, WITHOUT_ERR_CODE
INTR_VECTOR 0x0b, WITHOUT_ERR_CODE 
INTR_VECTOR 0x0c, WITH_ERR_CODE
INTR_VECTOR 0x0d, WITHOUT_ERR_CODE
INTR_VECTOR 0x0e, WITHOUT_ERR_CODE
INTR_VECTOR 0x0f, WITH_ERR_CODE 
INTR_VECTOR 0x10, WITH_ERR_CODE
INTR_VECTOR 0x11, WITHOUT_ERR_CODE
INTR_VECTOR 0x12, WITH_ERR_CODE
INTR_VECTOR 0x13, WITH_ERR_CODE 
INTR_VECTOR 0x14, WITH_ERR_CODE
INTR_VECTOR 0x15, WITH_ERR_CODE
INTR_VECTOR 0x16, WITH_ERR_CODE
INTR_VECTOR 0x17, WITH_ERR_CODE 
INTR_VECTOR 0x18, WITHOUT_ERR_CODE
INTR_VECTOR 0x19, WITH_ERR_CODE
INTR_VECTOR 0x1a, WITHOUT_ERR_CODE
INTR_VECTOR 0x1b, WITHOUT_ERR_CODE 
INTR_VECTOR 0x1c, WITH_ERR_CODE
INTR_VECTOR 0x1d, WITHOUT_ERR_CODE
INTR_VECTOR 0x1e, WITHOUT_ERR_CODE
INTR_VECTOR 0x1f, WITH_ERR_CODE 
INTR_VECTOR 0x20, WITH_ERR_CODE
