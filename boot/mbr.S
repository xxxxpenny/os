;主引导程序
;-----------------------------
%include "boot.inc"
section mbr vstart=0x7c00
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00
    mov ax,0xb800
    mov gs,ax

;清屏使用0x06
;-----------------------------
;输入
;ah = 0x06
    mov ax,0600h
    mov bx,0700h
    mov cx,0
    mov dx,184fh

    int 10h

    mov byte [gs:0x00],'1'
    mov byte [gs:0x01],0xA4

    mov byte [gs:0x02],' '
    mov byte [gs:0x03],0xA4
    
    mov byte [gs:0x04],'M'
    mov byte [gs:0x05],0xA4
    
    mov byte [gs:0x06],'B'
    mov byte [gs:0x07],0xA4 

    mov byte [gs:0x08],'R'
    mov byte [gs:0x09],0xA4

    mov eax,LOADER_START_SECTOR
    mov bx,LOADER_BASE_ADDR
;    mov cx,1  loader变大,读入4个扇区
    mov cx,4
    call rd_disk_m_16

    jmp LOADER_BASE_ADDR + 0x300

;-----读取硬盘 n个扇区----
rd_disk_m_16:
;----------------------
                ;eax=LBA扇区号
                ;bx=数据写入内存地址
                ;cx=读入的扇区数
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
.go_on_read:
    in ax,dx
    mov [bx],ax
    add bx,2
    loop .go_on_read
    ret


times 510-($-$$) db 0
db 0x55,0xaa






