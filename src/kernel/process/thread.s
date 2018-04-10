[bits 32]

section .text

global switch_to_thread

; void switch_to_thread(struct TCB *src, struct TCB *dst);
switch_to_thread:

    push esi
    push edi
    push ebx
    push ebp

    ; 取得src指针值
    mov eax, [esp + 20]
    ; 栈指针 -> src->ker_stack
    mov [eax], esp

    ; 取得dst指针值
    mov eax, [esp + 24]
    ; dst->ker_stack -> 栈指针
    mov esp, [eax]

    pop ebp
    pop ebx
    pop edi
    pop esi

    ret
