;00: 01 00 02 00 ff ff ff ff 01 00 ff ff 00 80 00 80
;10: 81 7f c3 ef 33 e9 01 ff ff ff 80 80 ff 01 7f 80
;20: 00 bc 00 00 01 80 00 80 17 53 bc 00 81 37 00 80
;30: XX XX XX XX 83 08 13 00 97 00 46 00 17 00 87 08
;40: 93 00 46 00 87 00 87 08 87 08 82 00 46 00 97 00
;50: 96 00 16 00 93 08 93 00 16 08 13 00 46 00 97 00

use16
start:

mov sp,96

; cmp word tests
mov ax,1
mov bx,2
cmp ax,bx
mov word[0],ax
mov word[2],bx
pushf
; #1

mov dx,0ffffh
mov word[4],0ffffh
cmp word[4],dx
mov word[6],dx
pushf
; #2

mov cx,0ffffh
mov word[8],1
cmp word[8],cx
mov word[10],cx
pushf
; #3

mov ax,08000h
cmp ax,00001h
mov word[12],ax
pushf
; #4

mov bp,8000h
db 083h,0fdh,0ffh
mov word[14],bp
pushf
; #5

mov si,07f81h
cmp si,0903ch
mov word[16],si
pushf
; #6

mov word[18],0efc3h
cmp word[18],0c664h
pushf
; #7

mov word[20],0e933h
dw 03e83h,00014h
db 064h
pushf
; #8

; cmp byte tests
mov byte[22],1
cmp byte[22],2
pushf
; #9

mov dh,0ffh
cmp dh,0ffh
mov word[23],dx
pushf
; #10

mov al,0ffh
cmp al,1
mov word[25],ax
pushf
; #11

mov byte[27],80h
mov ch,1
cmp ch,byte[27]
mov word[28],cx
pushf
; #12

mov bl,80h
mov byte[30],7Fh
cmp byte[30],bl
mov word[31],bx
pushf
; #13

mov al,0bch
mov ah,08eh
cmp ah,al
mov word[33],ax
pushf
; #14

; neg word tests
mov cx,0
neg cx
mov word[34],cx
pushf
; #15

mov word[36],7fffh
neg word[36]
pushf
; #16

mov bp,8000h
neg bp
mov word[38],bp
pushf
; #17

mov word[40],0ace9h
neg word[40]
pushf
; #18

; neg byte tests
mov ah,0
neg ah
mov word[42],ax
pushf
; #19

mov byte[44],7fh
neg byte[44]
pushf
; #20

mov cl,0c9h
neg cl
mov word[45],cx
pushf
; #21

mov byte[47],80h
neg byte[47]
pushf
; #22
hlt

rb 65520-$
jmp start
rb 65535-$
db 0ffh

