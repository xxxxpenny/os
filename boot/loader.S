%include "boot.inc"

section loader vstart=LOADER_BASE_ADDR
    LOADER_STACK_TOP equ LOADER_BASE_ADDR
    ; 构建 gdt 及其内部描述符
    GDT_BASE: dd 0x00000000
            dd 0x00000000
    CODE_DESC: dd 0x0000FFFF
            dd DESC_CODE_HIGH4
    DATA_STACK_DESC: dd 0x0000FFFF
            dd DESC_DATA_HIGH4
    VIDEO_DESC: dd 0x80000007
            dd DESC_VIDEO_HIGH4 ;32
    GDT_SIZE equ $ - GDT_BASE 
    GDT_LIMIT equ GDT_SIZE - 1 
    times 60 dq 0 ; 480
    ; 移动索引位置到高12位
    ; 相当于 ((CODE_DESC - GDT_BASE) / 8) << 3
    SELECTOR_CODE equ (0x0001<<3) + TI_GDT + RPL0 
    SELECTOR_DATA equ (0x0002<<3) + TI_GDT + RPL0 
    SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0 


    ; total_mem_bytes 总内存容量,单位为字节
    ; 当前偏移文件头为0x200字节
    ; 内存地址为0xb00 = 0x900 + 0x200
    total_mem_bytes dd 0

    ; 2 byte 的段限界,单位是 byte
    ; 4 byte 的段基址,32位地址
    gdt_ptr dw GDT_LIMIT
            dd GDT_BASE

    ;人工对齐 total_mem_bytes:4 + gdt_ptr:6 + ards_buf:244 + ards_nr:2 = 256
    ards_buf times 244 db 0
    ards_nr dw 0
    ; loadermsg db '2 loader in real.'
loader_start:
; int 15h eax=0000E820h,edx = 534D4150h 获取内存
    xor ebx, ebx
    mov edx,534d4150h
    mov di,ards_buf
    .e820_mem_get_loop:
        mov eax,0x0000e820
        mov ecx,20
        int 0x15
        jc .e820_failed_so_try_e801
        add di,cx
        inc word [ards_nr]
        cmp ebx,0
        jnz .e820_mem_get_loop
    ;找出所有 ards 结构中的最大值
    mov cx,[ards_nr]
    mov ebx,ards_buf
    xor edx,edx
    .find_max_mem_area:
        mov eax,[ebx] ;base_add_low
        add eax,[ebx+8] ;length_low
        add ebx,20
        cmp edx,eax
        jge .next_ards
        mov edx,eax
    .next_ards:
        loop .find_max_mem_area
        jmp .mem_get_ok

; int 0x15 ax=0xe801
    .e820_failed_so_try_e801:
       mov ax,0xe801
       int 0x15 
       jc .e801_failed_so_try88
        ; 先算第15m,ax,cx 的单位为 kb,转换为 m
        mov cx,0x400
        mul cx
        shl edx,16
        and eax,0x0000ffff
        or edx,eax
        add edx,0x100000
        mov esi,edx

        ; 再算16mb 以上的内存
        xor eax,eax
        mov ax,bx
        mov ecx,0x10000
        mul ecx
        add esi,eax
        mov edx,esi
        jmp .mem_get_ok

; int 15h ah=0x88
    .e801_failed_so_try88:
        mov ah,0x88
        int 15h
        jc .error_hlt
        and eax,0x0000ffff

        mov cx,0x400
        mul cx
        shl edx,16
        or edx,eax
        add edx,0x100000
.mem_get_ok:
    mov [total_mem_bytes],edx

;---------------
;打开 A20地址线
;加载 gdt
;将 cr0的 pe 位置1
;-----------------打开 A20-----------
    in al,0x92
    or al,0000_0010b
    out 0x92,al
;-----------------加载 gdt-----------
    lgdt [gdt_ptr]

;-----------------cr0的 pe 置为1
    mov eax,cr0
    or eax,0x00000001
    mov cr0,eax

    jmp dword SELECTOR_CODE:p_mode_start

.error_hlt:
    hlt

[bits 32]
p_mode_start:
    mov ax,SELECTOR_DATA
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp,LOADER_STACK_TOP
    mov ax,SELECTOR_VIDEO
    mov gs,ax


    ;---------加载 kernel
    mov eax,KERNEL_START_SECTOR
    mov ebx,KERNEL_BIN_BASE_ADDR

    mov ecx,200

    call rd_disk_m_32


    call setup_page

    ; 要将描述符地址及偏移量希望如内存 gdt_ptr,一会用新地址重新加载
    sgdt [gdt_ptr] ;存储到原来的 gdt 位置
    ; 拿段基址, 2 byte gdt_limit  
    mov ebx,[gdt_ptr+2]
    ; 第三个段 8byte * 3 = 24 = 0x18
    ; 端基址的31-24位在高32位,所以+4
    or dword [ebx+0x18+4],0xc0000000

    ; 将 gdt 的基址映射到高3G 空间
    add dword [gdt_ptr+2],0xc0000000

    add esp,0xc0000000

    mov eax,PAGE_DIR_TABLE_POS
    mov cr3,eax

    mov eax,cr0
    or eax,0x80000000
    mov cr0,eax

    ; 开启分页后,重新设置 gdt
    lgdt [gdt_ptr]

    jmp SELECTOR_CODE:enter_kernel

enter_kernel:
    call kernel_init
    mov esp,0xc009f000
    ; mov ecx,81
    ; mov esi,160
    ; .print:
    ;     mov byte [gs:esi],'V'
    ;     inc esi
    ;     mov byte [gs:esi],0x04
    ;     inc esi
    ;     loop .print
    jmp KERNEL_ENTRY_POINT




;--------------- 创建页目录与页表 ---------
;先把页目录占用清零
setup_page:
    mov ecx,4096
    mov esi,0
    .clear_page_dir:
        mov byte [PAGE_DIR_TABLE_POS+esi],0
        inc esi
        loop .clear_page_dir
    .create_pde:
        mov eax,PAGE_DIR_TABLE_POS
        add eax,0x1000
        ; 此处为 ebx 复值,是为.create_pte做准备,ebx 是基址
        mov ebx,eax
        ; 下面将页目录项0和0xc00都存为第一个页表的地址,每个页表表示4MB 内存
        ; 这样0xc03fffff一下的地址和0x003fffff一下的地址都指向相同的页表
        ; 为地址映射为内核地址做准备
        or eax,PG_US_U|PG_RW_W|PG_P
        mov [PAGE_DIR_TABLE_POS+0x0],eax
        mov [PAGE_DIR_TABLE_POS+0xc00],eax
        sub eax,0x1000
        ; 使最后一个页目录指向页表自己的位置
        mov [PAGE_DIR_TABLE_POS+4092],eax

        ;创建 pte
        ;目前只分配1m的内存空间 256 * 4k = 1m
        ;在0x0000000-0x000fffff的物理地址
        mov ecx,256
        mov esi,0
        mov edx,PG_US_U|PG_RW_W|PG_P
    .create_pte:
        mov [ebx+esi*4],edx

        add edx,4096
        inc esi
        loop .create_pte

        mov eax, PAGE_DIR_TABLE_POS
        add eax,0x2000
        or eax,PG_US_U|PG_RW_W|PG_P
        mov ebx,PAGE_DIR_TABLE_POS
        mov ecx,254
        mov esi,769
    .create_kernel_pde:
        mov [ebx+esi*4],eax
        inc esi
        add eax,0x1000
        loop .create_kernel_pde
        ret

;--------------读取硬盘-----------------
    ;eax=LBA扇区号
    ;ebx=数据写入地址
    ;ecx=读取扇区数
rd_disk_m_32:
    mov esi,eax
    mov di,cx
;设置读取硬盘扇区数
    mov dx,0x1f2
    mov al,cl
    out dx,al   ;扇区数

    mov eax,esi
;设置LBA的值
    mov dx,0x1f3
    out dx,al

    mov cl,8
    shr eax,cl
    mov dx,0x1f4
    out dx,al

    shr eax,cl
    mov dx,0x1f5
    out dx,al

    shr eax,cl
    and al,0x0f
    or al,0xe0
    mov dx,0x1f6
    out dx,al
;读入
    mov dx,0x1f7
    mov al,0x20
    out dx,al

.not_ready:
    nop
    in al,dx
    and al,0x88
    cmp al,0x08
    jnz .not_ready

    mov ax,di
    mov dx,256
    mul dx
    mov cx,ax
    mov dx,0x1f0
.go_on_ready:
    in ax,dx
    mov [ebx],ax
    add ebx,2
    loop .go_on_ready
    ret    
    

;------------初始化内核--------------
kernel_init:
    xor eax,eax
    xor ebx,ebx ;ebx 记录程序头表位置
    xor ecx,ecx ;cx 记录程序中 program header 数量 
    xor edx,edx ;dx 记录program header 尺寸,即e_phentsize

    ;e_phentsize
    mov dx,[KERNEL_BIN_BASE_ADDR+42]
    ;e_phoff
    mov bx,[KERNEL_BIN_BASE_ADDR+28]
    ; 第一个 program header的位置  0x34
    add ebx,KERNEL_BIN_BASE_ADDR
    mov cx,[KERNEL_BIN_BASE_ADDR+44]

.each_segment:
    cmp byte [ebx+0], PT_NULL
    je .PTNULL

    ;为函数 memcpy 压入参数,从右到左
    ; memcpy(dst, src, size)

    ;p_filesz
    push dword [ebx+16]
    ;p_offset
    mov eax,[ebx+4]
    add eax,KERNEL_BIN_BASE_ADDR
    push eax
    push dword [ebx+8]

    call mem_cpy
    add esp,12

.PTNULL:
    add ebx,edx
    loop .each_segment
    ret


;-------------- 逐字节 copy-------------
; 入栈三个参数 dst,src,size
; 输出:无
;-------------------------------------
mem_cpy:
    cld
    push ebp
    mov ebp,esp
    push ecx

    mov edi,[ebp+8] ;dst
    mov esi,[ebp+12];src
    mov ecx,[ebp+16];size
    rep movsb ;逐字节 copy

    pop ecx
    pop ebp
    ret






    