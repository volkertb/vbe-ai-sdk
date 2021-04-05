
;   /*\
;---|*|-----------------====< VBE/AI OPL2 INCLUDE >====----------------------
;---|*|
;---|*| Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    February 2, 1994. 1.00 release
;---|*|
;---|*| Additional Changes:
;---|*|   04/28 - Removed the vol1/vol0 bytes fields, patch pointer fields,
;---|*| 	  added sustain and voice flag fields
;---|*|   04/28 - Added compressed patch structures.
;---|*|   04/20 - Commented out HWF1_STEALING & increased dichmask to a word.
;---|*|   04/20 - Moved the compile time flags here, from hardware.asm
;---|*|   03/02 - Removed the silly reference to WAVE sample sizes.
;---|*|
;   \*/

;
; compile time flags
;

VELOCITY        equ     1       ; enables velocity to control note volume
BETTEROPL3      equ     1       ; enables 18 2op voice mode for 9 tones
 FNUMDELTA      equ     1       ; shift FNUM freq by X
 STEREOLEFT     equ     020h    ; left stereo channel bit
 STEREORIT      equ     010h    ; right stereo channel bit

;
; This driver supports the OPL2 9 melodic voice mode.
;

MAXTONES        equ     9               ; the opl2 driver supports 9 rhythm voices
FMADDRESS       equ     0388h           ; standard OPL2 address for Adlib
PERCUSSIVEKEY   equ     60              ; middle c for percussive instruments

;
; This structure is used by the MIDI driver to store up to 4 bytes in
; low memory. Only permanent, or global data should be stored in this
; location. The driver may use the 4 bytes for any purpose. The following
; useage was created for use by the OPL2 driver. It may be redefined for
; any use.
;

hwdata  struc
 hwpref         db      0       ; user preference
 hwflag1        db      0       ; bit flags
 hwaddr         db      0       ; base address of the chip
 hwfree2        db      0       ; available...
hwdata  ends

; global retained flags in 'hwflag1'

;;HWF1_STEALING equ 00010000b	; voice stealing is allowed
HWF1_OPL3       equ 00100000b   ; OPL3 chip

;
; Vendor specific data that will be appended to the end of the services
; structure.
;

;;PATCHLIST	  equ	  128
vendordata      struc

  dihandle      dw      0       ;       ; device handle
  dilasterr     db      0       ; dd 1  ; last error storage
  dirunning     db      0       ;       ; running status

  dichmask	dw	0	;	; channel locking mask
  diunused1     dw      0       ;       ; unused variable

  diidxloop     dw      0       ; dd 3
  didtaloop     dw      0       ;

  divoiceflag	db	MAXTONES dup(0) ; bit flags for voice control
  diunused2	db	0		; padding

  divoicefree   db      MAXTONES dup(0) ; free voice indicator
  ditones       db      0       ;       ; # of tones (minimum unit of sound)

  divoices      dw      MAXTONES dup(0) ; Table of channel/notes assigned to
                                        ; each voice.  Each entry contains:
                                        ;  byte 0: note #, byte 1: channel #
                                        ; The table is indexed by voice #.
  divtimes      dw      MAXTONES dup(0) ; voice active countdown timers

  dichvol       db      16       dup(0) ; controller channel volumes

  dicpatch      db      16 dup(0)       ; current patch for each channel

;;dipatchnum	dw	PATCHLIST dup(0); the patch # matching the next ptrs.
;;dipatchptr	dd	PATCHLIST dup(0); a table of far * to the apps patches.

  dimsglen      db      0               ; count of bytes into a midi msg
  dimsgidx      dw      0               ; no count
  dimsg         db      3 dup(0)        ; holds up to 3 bytes of a midi message

  dirpn         dw      16 dup(0)       ; registered parameter ID #s
  dipitchres    dw      16 dup(0)       ; pitch resolution set with controllers

  dibender      db      16 dup(0)       ; channel bender offsets - MSB only
  dibendmul     dw      16 dup(0)       ; FNUM delta multiplier

  dipercfreq    db      128 dup(0)      ; holds the key # of percussive patches

  disustain	dw	0		; sustain active bit fields

  diends        db      0               ; end of structure

vendordata      ends

;
; divoiceflag bit definitions
;
DIVF_SUST	equ	00000001b	; voice is sustained
DIVF_UNUSED2	equ	00000010b	;
DIVF_UNUSED4	equ	00000100b	;
DIVF_UNUSED8	equ	00001000b	;
DIVF_UNUSED10	equ	00010000b	;
DIVF_UNUSED20	equ	00100000b	;
DIVF_UNUSED40	equ	01000000b	;
DIVF_UNUSED80	equ	10000000b	;

;
; The new, compressed patch structure
;

BNK             STRUC
 o1_AVEKM	db ?		; AM VIB EM KSR MULT
 o1_KSLTL	db ?		; KSL TL
 o1_AD		db ?		; Attack/Decay
 o1_SR		db ?		; Sustain/Release
 o2_AVEKM	db ?		; AM VIB EM KSR MULT
 o2_KSLTL	db ?		; KSL TL
 o2_AD		db ?		; Attack/Decay
 o2_SR		db ?		; Sustain/Release
 opfb_c 	db ?		; Feedback, Connection bit, WS1, WS0
 opfreq 	db ?		; Percussive FREQuency
BNK		ENDS

;
; single operator structure
;
BNKop		STRUC
 op_AVEKM    db ?
 op_KSLTL    db ?
 op_AD	     db ?
 op_SR	     db ?
BNKop		ENDS

;
; The memreq field in the services structure has added 100h in
; length for shadowing the OPL2 registers.
;

NUMOPL2REGS        equ  100h

opl2mem struc
 midiservices   db      (size MIDIService) dup(0)
 msvendor       db      (size vendordata) dup(0); all the needed working mem
 fmregs         db      NUMOPL2REGS dup (0)     ; shadow the FM registers
opl2mem ends

OCTAVE_0        equ     (00h SHL 8)     ; Note C0
OCTAVE_1        equ     (04h SHL 8)     ;      C1
OCTAVE_2        equ     (08h SHL 8)     ;      C2
OCTAVE_3        equ     (0ch SHL 8)     ;      C3
OCTAVE_4        equ     (10h SHL 8)     ;      C4
OCTAVE_5        equ     (14h SHL 8)     ;      C5
OCTAVE_6        equ     (18h SHL 8)     ;      C6
OCTAVE_7        equ     (1ch SHL 8)     ;      C7

noteC           equ     342
noteCSHARP      equ     363
noteD           equ     384
noteDSHARP      equ     407
noteE           equ     432
noteF           equ     457
noteFSHARP      equ     484
noteG           equ     513
noteGSHARP      equ     544
noteA           equ     576
noteASHARP      equ     611
noteB           equ     647

;
; FNUM values within an octave
;

NoteTable       label   word
        dw      noteC           ;  C
        dw      noteCSHARP      ;  C#
        dw      noteD           ;  D
        dw      noteDSHARP      ;  D#
        dw      noteE           ;  E
        dw      noteF           ;  F
        dw      noteFSHARP      ;  F#
        dw      noteG           ;  G
        dw      noteGSHARP      ;  G#
        dw      noteA           ;  A
        dw      noteASHARP      ;  A#
        dw      noteB           ;  B
;
; table of FNUM deltas within an octave
;

DeltaTable       label   byte
        db      21/2            ; 1 C   The value is divided by two to give
        db      21/2            ; 2 C#  a single semi-tone delta between the
        db      23/2            ; 3 D   FNUM values.
        db      25/2            ; 4 D#
        db      25/2            ; 5 E
        db      27/2            ; 6 F
        db      29/2            ; 7 F#
        db      31/2            ; 8 G
        db      32/2            ; 9 G#
        db      35/2            ;10 A
        db      36/2            ;11 A#
        db      39/2            ;12 B

;
; features supported by an OPL2 chip
;

FEATURES = 0

;   /*\
;---|*| end of OPL2.INC
;   \*/

