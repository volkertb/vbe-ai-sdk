
;   /*\
;---|*|-----------------====< VESA AI WAVE INCLUDE >====----------------------
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    February 2, 1994. 1.00 release
;---|*|
;   \*/

;
; A structure listing the DMA I/O addresses for the different channels
;

dmaio   struc
 _dmach         db      ?       ; DMA channel number
 _dmawrsmr      db      ?       ; DMA write single mask register
 _dmawrmode     db      ?       ; DMA write mode register
 _dmapg         db      ?       ; DMA page register
 _dmaadr        db      ?       ; DMA address
 _dmaclr        db      ?       ; DMA clear low/high flip-flop
dmaio   ends

;
; data on the stack when calling API.ASM
;
parmw1        equ     [bp+2+@farret+0]  ; 1st parameter word on the stack
parmw2        equ     [bp+2+@farret+2]  ; 2nd parameter word on the stack
parmw3        equ     [bp+2+@farret+4]  ; 3rd parameter word on the stack
parmw4        equ     [bp+2+@farret+6]  ; 4th parameter word on the stack
parmw5        equ     [bp+2+@farret+8]  ; 5th parameter word on the stack
parmw6        equ     [bp+2+@farret+10] ; 6th parameter word on the stack
parmw7        equ     [bp+2+@farret+12] ; 7th parameter word on the stack

;   /*\
;---|*| end of WAVE.INC
;   \*/


