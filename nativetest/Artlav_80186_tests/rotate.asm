use16
start:

; rcl word operations
mov ax,03b5eh
mov bx,0c8a7h
mov word[0],02072h
mov word[2],03e79h

mov sp,160

rcl  ax,1        ; (1)
pushf
; #1
mov word[32],ax

rcl word[0],1        ; (2)
pushf
; #2

mov cx,0100h
rcl bx,cl		; [3], zero bit shift
pushf
; #3
mov word[34],bx

mov cx,0ffffh
mov dx,bx
rcl dx,cl		; [3], -1, result 0
pushf
; #4
mov word[36],dx

mov cl,08h
rcl bx,cl		; [3] normal
pushf
; #5
mov word[38],bx

mov cl,04h
rcl word[2],cl		; (4)
pushf
; #6

; rcl byte operations
mov dx,05904h
mov ax,0be7ch
mov word[4],0d62fh
mov word[6],06fd8h

rcl  ah,1        ; (5)
pushf
; #7
mov word[40],ax

rcl byte[5],1        ; (6)
pushf
; #8

mov cl,07h
rcl dl,cl		; [7]
pushf
; #9
mov word[42],dx

rcl byte[6],cl		; (8)
pushf
; #10

; rcr word operations
mov ax,015d6h
mov bx,08307h
mov word[8],09ab7h
mov word[10],028b6h

rcr  ax,1        ; (9)
pushf
; #11
mov word[44],ax

rcr word[8],1        ; (10)
pushf
; #12

mov cx,0100h
rcr bx,cl		; [11], zero bit shift
pushf
; #13
mov word[46],bx

mov cx,0ffffh
mov dx,bx
rcr dx,cl		; [11], -1, result 0
pushf
; #14
mov word[48],dx

mov cl,05h
rcr bx,cl		; [11] normal
pushf
; #15
mov word[50],bx

mov cl,04h
rcr word[10],cl		; (12)
pushf
; #16

; rcr byte operations
mov dx,07eaah
mov ax,03a8dh
mov word[12],0a414h
mov word[14],02838h

rcr  ah,1        ; (13)
pushf
; #17
mov word[52],ax

rcr byte[13],1       ; (14)
pushf
; #18

mov cl,07h
rcr dl,cl		; [15]
pushf
; #19
mov word[54],dx

rcr byte[14],cl		; (16)
pushf
; #20

; rol word operations
mov ax,0020dh
mov bx,08d5ah
mov word[16],028ddh
mov word[18],0d74ah

rol  ax,1        ; (17)
pushf
; #21
mov word[56],ax

rol word[16],1       ; (18)
pushf
; #22

mov cx,0100h
rol bx,cl		; [19], zero bit shift
pushf
; #23
mov word[58],bx

mov cx,0ffffh
mov dx,bx
rol dx,cl		; [19], -1, result 0
pushf
; #24
mov word[60],dx

mov cl,04h
rol bx,cl		; [19] normal
pushf
; #25
mov word[62],bx

mov cl,04h
rol word[18],cl		; (20)
pushf
; #26

; rol byte operations
mov dx,09d09h
mov ax,0c948h
mov word[20],00b80h
mov word[22],048e8h

rol  ah,1        ; (21)
pushf
; #27
mov word[64],ax

rol byte[21],1       ; (22)
pushf
; #28

mov cl,07h
rol dl,cl		; [23]
pushf
; #29
mov word[66],dx

rol byte[22],cl		; (24)
pushf
; #30


; ror word operations
mov ax,0f25eh
mov bx,02eb5h
mov word[24],00151h
mov word[26],07237h

ror  ax,1        ; (25)
pushf
; #31
mov word[68],ax

ror word[24],1       ; (26)
pushf
; #32

mov cx,0100h
ror bx,cl		; [27], zero bit shift
pushf
; #33
mov word[70],bx

mov cx,0ffffh
mov dx,bx
ror dx,cl		; [27], -1, result 0
pushf
; #34
mov word[72],dx

mov cl,04h
ror bx,cl		; [27] normal
pushf
; #35
mov word[74],bx

mov cl,04h
ror word[26],cl		; (28)
pushf
; #36

; ror byte operations
mov dx,04288h
mov ax,08babh
mov word[28],05dd9h
mov word[30],0c7f7h

ror  ah,1        ; (29)
pushf
; #37
mov word[76],ax

ror byte[29],1       ; (30)
pushf
; #38

mov cl,07h
ror dl,cl		; [31]
pushf
; #39
mov word[78],dx

ror byte[30],cl		; (32)
pushf
; #40


hlt

rb 65520-$
jmp start
rb 65535-$
db 0ffh
