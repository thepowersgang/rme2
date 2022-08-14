;00: ff ff 02 00 00 00 ff ff  01 00 fe ff ff 7f 01 80
;10: 45 ef 5f 29 cf e8 ff ff  00 fe 7f 80 fe 81 ff 80
;20: 00 bc d2 01 00 00 00 00  00 ff ff 01 00 fe ff ff
;30: 7f 01 80 7a 70 8b c4 90  fd fe ff ff fd 7f 80 fe
;40: 81 7e 80 00 b9 1a ff ff  ff 7f 11 74 ff 7f b4 00
;50: 00 00 00 00 00 00 00 00  00 00 00 00 86 00 12 08
;60: 96 00 06 00 16 08 96 00  12 00 06 00 87 08 82 00
;70: 97 00 93 00 87 00 86 00  13 00 93 00 16 08 82 00
;80: 46 00 46 00 87 00 87 08  87 08 82 00 46 00 97 00
;90: 96 00 16 00 93 08 93 00  16 08 82 00 46 00 97 00
; ^ from qemu
use16
start:

%macro rb 1
times %1+$$ db 0
%endmacro

mov sp,160

; sub word tests
mov ax,00001h
mov bx,00002h
sub ax,bx
mov word[0],ax
mov word[2],bx
pushf
; #1

mov dx,0ffffh
mov word[4],0ffffh
sub word[4],dx
mov word[6],dx
pushf
; #2

mov cx,0ffffh
mov word[8],00001h
sub cx,word[8]
mov word[10],cx
pushf
; #3

mov ax,08000h
sub ax,00001h
mov word[12],ax
pushf
; #4

mov bp,08000h
db 083h,0edh,0ffh
mov word[14],bp
pushf
; #5

mov si,07f81h
sub si,0903ch
mov word[16],si
pushf
; #6

mov word[18],0efc3h
sub word[18],0c664h
pushf
; #7

mov word[20],0e933h
dw 02e83h, 00014h
db 064h
pushf
; #8

; sub byte tests
mov byte[22],001h
sub byte[22],002h
pushf
; #9

mov dh,0ffh
sub dh,0ffh
mov word[23],dx
pushf
; #10

mov al,0ffh
sub al,001h
mov word[25],ax
pushf
; #11

mov byte[27],080h
mov ch,001h
sub ch,byte[27]
mov word[28],cx
pushf
; #12

mov bl,080h
mov byte[30],07fh
sub byte[30],bl
mov word[31],bx
pushf
; #13

mov al,0bch
mov ah,08eh
sub ah,al
mov word[33],ax
pushf
; #14

; sbb word tests
mov ax,00001h
mov bx,00002h
sbb bx,ax
mov word[35],ax
mov word[37],bx
pushf
; #15

mov dx,0ffffh
mov word[39],0ffffh
sbb word[39],dx
mov word[41],dx
pushf
; #16

mov cx,0ffffh
mov word[43],00001h
sbb cx,word[43]
mov word[45],cx
pushf
; #17

mov ax,08000h
sbb ax,00001h
mov word[47],ax
pushf
; #18

mov bp,08000h
db 083h,0ddh,0ffh
mov word[49],bp
pushf
; #19

mov si,052c3h
sbb si,0e248h
mov word[51],si
pushf
; #20

mov word[53],0e74ch
sbb word[53],022c0h
pushf
; #21

mov word[55],0fd85h
dw 01e83h, 00037h
db 0f5h
pushf
; #22

; sbb byte tests
mov byte[57],001h
sbb byte[57],002h
pushf
; #23

mov dh,0ffh
sbb dh,0ffh
mov word[58],dx
pushf
; #24

mov al,0ffh
sbb al,001h
mov word[60],ax
pushf
; #25

mov byte[62],080h
mov ch,001h
sbb ch,byte[62]
mov word[63],cx
pushf
; #26

mov bl,080h
mov byte[65],0ffh
sbb byte[65],bl
mov word[66],bx
pushf
; #27

mov al,0b9h
mov ah,0d3h
sbb ah,al
mov word[68],ax
pushf
; #28

; dec word tests
mov di,00000h
dec di
mov word[70],di
pushf
; #29

mov bp,08000h
db 0ffh, 0cdh
mov word[72],bp
pushf
; #30

mov word[74],07412h
dec word[74]
pushf
; #31

; dec byte tests
mov dl,000h
dec dl
mov word[76],dx
pushf
; #32

mov byte[77],080h
dec byte[77]
pushf
; #33

mov byte[78],0b5h
dec byte[78]
pushf
; #34
hlt

rb 65520-$
jmp start
rb 65535-$
db 0ffh

