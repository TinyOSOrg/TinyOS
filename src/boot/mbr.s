SECTION MBR vstart=0x7c00
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov sp, 0x7c00

	mov ax, 0x0600
	mov bx, 0x0700
	mov cx, 0
	mov dx, 0x184f
	int 0x10

	mov ah, 3
	mov bh, 0
	int 0x10

	mov ax, message
	mov bp, ax
	mov cx, 10
	mov ax, 0x1301

	mov bx, 0x2
	int 0x10

	mov ax, 'M'
	mov bl, 0
	mov bh, 0
	call print_char

	mov ax, 'i'
	mov bl, 0
	mov bh, 1
	call print_char

	mov ax, 'n'
	mov bl, 0
	mov bh, 2
	call print_char

	mov ax, 'e'
	mov bl, 0
	mov bh, 3
	call print_char

	jmp $

; al存字符ASCII码
; bl存行，bh存列
print_char:
	push ds

	; 设置ds
	push ax
	mov ax, 0xB000
	mov ds, ax
	pop ax

	; 计算输出地址
	push cx
	push ax
	mov al, bl
	mov cl, 80
	mul cl
	movzx cx, bh
	add ax, cx
	shl ax, 1
	pop cx ; 把之前的ax放到cx中

	push bx
	mov bx, ax
	mov [bx + 0x8000], cl
	mov ax, 0x0f
	mov [bx + 0x8001], ax
	pop bx

	mov ax, cx
	pop cx

	pop ds

	ret
	
	message db "Hello, OS!"
	times 510 - ($ - $$) db 0
	db 0x55, 0xaa
