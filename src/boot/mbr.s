%include "src/boot/boot.s"

section MBR vstart=0x7c00
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov sp, 0x7c00

	; 清屏
	mov ax, 0x0600
	mov bx, 0x0700
	mov cx, 0
	mov dx, 0x184f
	int 0x10

	; 加载bootloader到 BOOTLOADER_START_ADDR 并执行之
	mov eax, BOOTLOADER_START_SECTOR
	mov bx, BOOTLOADER_START_ADDR
	mov cx, BOOTLOADER_SECTOR_COUNT
	call read_disk

	jmp BOOTLOADER_START_ADDR

	jmp $

; eax：LBA
; bx ：目的地址
; cx ：扇区数目

read_disk:
    ; 备份eax和cx，等会儿会覆盖这俩
    mov esi, eax
    mov di, cx
    
    ; 设置扇区数
    mov dx, 0x1f2
    mov al, cl
    out dx, al
    mov eax, esi
    
    ; 写入LBA的低三个字节
    mov dx, 0x1f3
    out dx, al
    
    mov cl, 8
    shr eax, cl
    mov dx, 0x1f4
    out dx, al
    
    shr eax, cl
    mov dx, 0x1f5
    out dx, al
    
    ; 写入Device端口，用al来构造其值
    shr eax, cl
    and al, 0x0f
    or al, 0xe0
    mov dx, 0x1f6
    out dx, al
    
    ; 写Command
    ; 0x20为读，0x30为写
    mov dx, 0x1f7
    mov al, 0x20
    out dx, al
    
    ; 轮询硬盘状态直到数据准备就绪
waiting_disk_reading:
    in al, dx    ; Command和Status是同一个端口号，无需准备dx
    and al, 0x88 ; 提取第3位和第7位
    cmp al, 0x8  ; 第3位为1，第7位为0
    jnz waiting_disk_reading
    
    ; 从Data（0x1f0）端口读数据
    
    ; 计算读取次数
    ; 一次读两个字节，一个扇区需要读256次
    mov ax, di
    mov dx, 256
    mul dx
    mov cx, ax
    
    mov dx, 0x1f0
read_from_disk_port_to_mem:
    in ax, dx
    mov [bx], ax
    add bx, 2
    loop read_from_disk_port_to_mem
    
    ret
	
	times 510 - ($ - $$) db 0
	db 0x55, 0xaa
