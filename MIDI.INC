
;   /*\
;---|*|-----------------====< VBE/AI MIDI INCLUDE >====----------------------
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    February 2, 1994. 1.00 release
;---|*|
;   \*/

;   /*\
;---|*|---====< OPL2 Patch Structure >====----
;   \*/

opl2opr   struc
 opl2ksl        db      ?       ; ksl
 opl2freqMult   db      ?       ; freqMult
 opl2feedBack   db      ?       ; feedBack      // used by operator 0 only
 opl2attack     db      ?       ; attack
 opl2sustLevel  db      ?       ; sustLevel
 opl2sustain    db      ?       ; sustain
 opl2decay      db      ?       ; decay
 opl2release    db      ?       ; release
 opl2output     db      ?       ; output
 opl2am         db      ?       ; am
 opl2vib        db      ?       ; vib
 opl2ksr        db      ?       ; ksr
 opl2fm         db      ?       ; fm            // used by operator 0 only
opl2opr   ends

opl2native struc
 opl2type       dw      ?                   ; patch type
 opl2mode       db      ?                   ; mode;      0 = melodic, 1 = percussive
 opl2percVoice  db      ?                   ; percVoice; if mode == 1, voice number to be used
 opl2op0        db (size opl2opr) dup (?)   ; op0;
 opl2op1        db (size opl2opr) dup (?)   ; op1;
 opl2wave0      db      ?                   ; wave0;    waveform for operator 0
 opl2wave1      db      ?                   ; wave1;    waveform for operator 1
opl2native ends

;   /*\
;---|*|---====< OPL3 Patch Structure >====----
;   \*/

opl3opr    struc
 opl3type       dw      ?               ;   patch type
 opl3reg20      db      4 dup(?)        ;  register 20 four operator data
 opl3reg40      db      4 dup(?)        ;  register 40 four operator data
 opl3reg60      db      4 dup(?)        ;  register 60 four operator data
 opl3reg80      db      4 dup(?)        ;  register 80 four operator data
 opl3regC0      db      2 dup(?)        ;  register C0 two voice     data
 opl3regE0      db      2 dup(?)        ;  register E0 two voice     data
opl3opr    ends

;
; The following structure represents the format of whole patch passed in
; from the application. This whole structure makes up a single patch in
; our library. It includes the VESA patch header wrapper.
;

vbepatch struc
 vbehdr          db  size VAIDhdr   dup(0)  ; the header
 native          db  0                      ; native patch data
vbepatch ends

;
; data on the stack when calling API.ASM
;

parmw1          equ   [bp+2+@farret+0]  ; 1st parameter word on the stack
parmw2          equ   [bp+2+@farret+2]  ; 2nd parameter word on the stack
parmw3          equ   [bp+2+@farret+4]  ; 3rd parameter word on the stack
parmw4          equ   [bp+2+@farret+6]  ; 4th parameter word on the stack
parmw5          equ   [bp+2+@farret+8]  ; 5th parameter word on the stack
parmw6          equ   [bp+2+@farret+10] ; 6th parameter word on the stack

;   /*\
;---|*| end of MIDI.INC
;   \*/


