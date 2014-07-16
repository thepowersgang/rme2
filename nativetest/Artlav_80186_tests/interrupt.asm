;00h: 00 01 02 03 04 05 06 XX D7 0c d7 0E 02 40 FF 04
;10h: 00 00 00 f0 

use16
start:
mov dx,0
mov ds,dx
mov sp,1000h
mov sp,ss
mov word[52],0ebe0h
mov word[54],0e342h

mov ax,0effh
push ax
popf
mov byte [0],0
int 13                 ; (1)
mov byte [2],2
jmp ax

times 0cd7h-($-$$) db 0
mov byte[3],3
pushf
pop bx
mov word[12],0ebe0h
mov word[14],0e342h

int 3                  ; (2)
mov byte[4],4
mov word[16],03001h
mov word[18],0f000h

into                    ; (3) branch taken
hlt

times 02000h-($-$$) db 0
mov byte[1],1
pushf
pop ax
clc
iret                    ; (4)

times 03001h-($-$$) db 0
mov byte[5],5
pop cx
mov cx,4002h
push cx
iret

times 04002h-($-$$) db 0
mov byte[6],6
mov dx,4ffh
push dx
popf
mov word[16],5000h

into                    ; (3) branch not taken
mov word[8],ax
mov word[10],bx
mov word[12],cx
mov word[14],dx
mov word[16],sp
hlt 

times 05000h-($-$$) db 0
hlt

times 65520-($-$$) db 0
jmp start
times 65535-($-$$) db 0xFF
db 0ffh
