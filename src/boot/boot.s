;=====================================================
; mbr如何加载bootloader

; loader被加载到内存中的位置
BOOTLOADER_START_ADDR   equ 0x200
; loader从哪个扇区开始
BOOTLOADER_START_SECTOR equ 0x1
; 加载loader的时候读多少个扇区
BOOTLOADER_SECTOR_COUNT equ 0x4

;=====================================================
; kernel

KERNEL_START_ADDR   equ 0x70000
KERNEL_START_SECTOR equ 9
KERNEL_SECTOR_COUNT equ 200

;=====================================================
; 总内存容量存放在哪
; 4个字节

TOTAL_MEMORY_SIZE_ADDR equ 0x200

;=====================================================
; 全局描述符表

;-----------------------------------------------------
; 段描述符，按平坦模式定义

; G字段，为1表示segment limit单位为4K
SEGMENT_DESC_G_4K equ 0_1_000_00000_00000_00000_00000b

; D字段，为1表示默认操作均按32位进行
SEGMENT_DESC_D_32 equ 00_1_00_00000_00000_00000_00000b

; L字段，不开64位代码段，设置为0
SEGMENT_DESC_L_32 equ 000_0_0_00000_00000_00000_00000b

; AVL字段
SEGMENT_DESC_AVL  equ 0000_0_00000_00000_00000_00000b

; 段界限
SEGMENT_DESC_LIMIT_CODE  equ 1111_0_00000_00000_00000b
SEGMENT_DESC_LIMIT_DATA  equ 1111_0_00000_00000_00000b
SEGMENT_DESC_LIMIT_VIDEO equ 0000_0_00000_00000_00000b

SEGMENT_DESC_P equ 1_00000_00000_00000b

; 特权级0-3，0为最高特权
SEGMENT_DESC_DPL_0 equ 00_000_00000_00000b
SEGMENT_DESC_DPL_1 equ 01_000_00000_00000b
SEGMENT_DESC_DPL_2 equ 10_000_00000_00000b
SEGMENT_DESC_DPL_3 equ 11_000_00000_00000b

; 系统段/非系统段
SEGMENT_DESC_S_SYS    equ 0_00_00000_00000b
SEGMENT_DESC_S_NONSYS equ 1_00_00000_00000b

; 代码段：可执行，非一致，不可读，accessed为false
SEGMENT_DESC_TYPE_CODE  equ 1000_00000000b
; 数据段，不可执行，扩展方向向上，可写，accessed为false
SEGMENT_DESC_TYPE_DATA  equ 0010_00000000b
; 显存段
SEGMENT_DESC_TYPE_VIDEO equ 0010_00000000b

SEGMENT_DESC_CODE_HIGH equ (0x00 << 24) + \
                           SEGMENT_DESC_G_4K + \
                           SEGMENT_DESC_D_32 + \
                           SEGMENT_DESC_L_32 + \
                           SEGMENT_DESC_AVL + \
                           SEGMENT_DESC_LIMIT_CODE + \
                           SEGMENT_DESC_P + \
                           SEGMENT_DESC_DPL_0 + \
                           SEGMENT_DESC_S_NONSYS + \
                           SEGMENT_DESC_TYPE_CODE + \
                           0X00

SEGMENT_DESC_DATA_HIGH equ (0x00 << 24) + \
                           SEGMENT_DESC_G_4K + \
                           SEGMENT_DESC_D_32 + \
                           SEGMENT_DESC_L_32 + \
                           SEGMENT_DESC_AVL + \
                           SEGMENT_DESC_LIMIT_DATA + \
                           SEGMENT_DESC_P + \
                           SEGMENT_DESC_DPL_0 + \
                           SEGMENT_DESC_S_NONSYS + \
                           SEGMENT_DESC_TYPE_DATA + \
                           0X00

SEGMENT_DESC_VIDEO_HIGH equ (0x00 << 24) + \
                            SEGMENT_DESC_G_4K + \
                            SEGMENT_DESC_D_32 + \
                            SEGMENT_DESC_L_32 + \
                            SEGMENT_DESC_AVL + \
                            SEGMENT_DESC_LIMIT_VIDEO + \
                            SEGMENT_DESC_P + \
                            SEGMENT_DESC_DPL_0 + \
                            SEGMENT_DESC_S_NONSYS + \
                            SEGMENT_DESC_TYPE_VIDEO + \
                            0X0b

;-----------------------------------------------------
; 段选择子

; 特权级
SEGMENT_SELECTOR_ATTRIB_RPL_0 equ 00b
SEGMENT_SELECTOR_ATTRIB_RPL_1 equ 01b
SEGMENT_SELECTOR_ATTRIB_RPL_2 equ 10b
SEGMENT_SELECTOR_ATTRIB_RPL_3 equ 11b

SEGMENT_SELECTOR_ATTRIB_USE_LDT equ 100b
SEGMENT_SELECTOR_ATTRIB_USE_GDT equ 000b

;=====================================================
; 分页

PAGE_DIR_ENTRY_ADDR equ 0x100000

PAGE_PRESENT_TRUE  equ 1b
PAGE_PRESENT_FALSE equ 0b

PAGE_READ_WRITE_READ_ONLY  equ 00b
PAGE_READ_WRITE_READ_WRITE equ 10b

PAGE_USER_USER  equ 100b
PAGE_USER_SUPER equ 000b
