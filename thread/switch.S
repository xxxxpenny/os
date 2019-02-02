[bits 32]
section .text
global switch_to
; next pointer
; cur pointer
; ret
; esi
; edi
; ebx
; ebp
switch_to:
  push esi
  push edi
  push ebx
  push ebp
  mov eax,[esp+20]
  mov [eax],esp

  ;----------备份当前线程
  mov eax,[esp+24]
  mov esp,[eax]

  pop ebp
  pop ebx
  pop edi
  pop esi
  ret