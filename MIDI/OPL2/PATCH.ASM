
        page    63,131
	Title	VESA Sound Driver Patch table and loading code
        Subttl  Copyright (c) 1993,1994  V.E.S.A, Inc. All Rights Reserved.

;   /*\
;---|*|----------------------====< PATCH.ASM >====-------------------------
;---|*|
;---|*| Copyright (c) 1994  V.E.S.A, Inc. All Rights Reserved.
;---|*|
;---|*| VBE/AI 1.0 Specification
;---|*|    April 18th, 1994. 1.00 release
;---|*|
;---|*| This module has been written to compress an OPL2 2 operator FM
;---|*| patch into a smaller structure. This patch is then ready for
;---|*| internal use, i.e., ready to be written to the device.
;---|*|
;---|*| Additional changes:
;---|*|     04/28 - First iteration
;---|*|
;   \*/
;
        .286
        .model  small,c
        .code

	include vbeai.inc		  ; VESA include file
	include midi.inc		  ; VESA MIDI include file
	include midi\opl2\opl2.inc	  ; OPL2 include file
	include midi\opl2\tools\patch.inc ; OPL2 include file

	extrn	wrtIdxDta	:near	  ; write index/data
	extrn	wrtIdxDta0	:near	  ; write index/data to left
	extrn	wrtIdxDta2	:near	  ; write index/data to right
	extrn	driverdata	:byte	  ; persistent data

;
;   /*\
;---|*|-----------------------====< loadpatch >====--------------------------
;---|*|
;---|*| Load a patch into the hardware
;---|*|
;---|*| Entry Conditions:
;---|*|    BX holds the voice #
;---|*|    CL holds the patch #
;---|*|    CH holds the channel #
;---|*|    ES:SI point to a VBE/AI patch block with a MIDI_PATCH_OPL2
;---|*|         patch block nested within it.
;---|*|
;---|*| Exit Condtions:
;---|*|    working registers modified
;---|*|
;   \*/

VoiceOp label   byte
        db      0,3     ; voice 1 operators
        db      1,4     ; voice 1 operators
        db      2,5     ; voice 1 operators
        db      8,11    ; voice 1 operators
        db      9,12    ; voice 1 operators
        db      10,13   ; voice 1 operators
        db      16,19   ; voice 1 operators
        db      17,20   ; voice 1 operators
        db      18,21   ; voice 1 operators

holding db	(size BNK) dup(0)	; a translated patch structure

;
;   /*\
;---|*|----------------------====< loadpatch >====--------------------------
;---|*|
;---|*| Load a patch into the hardware
;---|*|
;---|*| Entry Conditions:
;---|*|    BX holds the voice #
;---|*|    CL holds the patch #
;---|*|    CH holds the channel #
;---|*|    SI points to a compressed patch block
;---|*|
;---|*| Exit Condtions:
;---|*|    working registers modified
;---|*|
;   \*/

	public	loadpatch
loadpatch	proc	near
	push	es
	push	si
        push    di
;
; set the patch number now loaded for this channel. Then setup the patch
; frequency if this is a percussive channel.

;;  BREAKPOINT

        mov     di,cx                   ; DI holds the channel #
        shr     di,8
        and     di,000FH                ; isolate the channel #

        cmp     di,9                    ; if not channel 9, skip special cases
        jnz     @F

        push    bx                      ; We will be saving any note frequency
        mov     bl,cl                   ; stored in the downloaded percussive
        and     bx,007Fh                ; patch
	mov	al,es:[si.opfreq]	; get the frequency
	mov	[msvendor.dipercfreq.bx],al
        pop     bx
	or	cl,80h			; force the patch # to be 128+
    ;
    @@:
	mov	[msvendor.dicpatch.di],cl; save the patch # for this channel
;
; setup to process the first operator
;
        shl     bx,1                    ; word index
	mov	cx,wptr VoiceOp[bx]	; cl=OP0, ch=OP1
        shr     bx,1
;
; Okay,  BX=voice, CL=operator, DL=wave sel, ES:SI point to the block
;
        push    si

	mov	dl,00000011b		; get op0 wave select bits
	and	dl,es:[si.opfb_c]	; get the wave select
	lea	si,es:[si.o1_AVEKM]	; point to the operator
	call	newloadoperator 	; load operator 0

        pop     si                      ; point to the whole block
;
; setup to process the second operator
;
        xchg    ch,cl
;
; Again, BX=voice, CL=operator, DL=wave, ES:SI point to the block
;
        push    si
	mov	dl,00001100b		; get op1 wave select bits
	and	dl,es:[si.opfb_c]
	shr	dl,2
	lea	si,es:[si.o2_AVEKM]	; point to the operator
	call	newloadoperator 	; load operator 1
        pop     si                      ; restore SI
;
; load the connect and feed back
;
	mov	ah,es:[si.opfb_c]	; bit are: 11123344
	shr	ah,4			;	   fffcw1w0

 if BETTEROPL3

        mov     al,bl                   ; voice! (not operator)
        add     al,0C0h

        test    cs:[driverdata.hwflag1],HWF1_OPL3 ; OPL3?
        jz      @F                      ; no, skip the right side

        or      ah,STEREOLEFT           ; set left channel bit
        call    wrtIdxDta2
        xor     ah,STEREOLEFT+STEREORIT ; flip to right channel bit
    ;
    @@:
        call    wrtIdxDta0
 else

        mov     al,bl                   ; voice! (not operator)
        add     al,0C0h
        call    wrtIdxDta

 endif

;
; now, program the volumes to allow sound...
;
        pop     di
	pop	si
	pop	es
        ret

loadpatch	endp

;
;   /*\
;---|*|---------------------====< newloadoperator >====------------------------
;---|*|
;---|*| Load one of the operators in the 2op voice
;---|*|
;---|*| Entry Conditions:
;---|*|    DS points to the applications copy of the services structure.
;---|*|    ES:SI points to the operator patch.
;---|*|    DI=channel #
;---|*|    BX=voice #
;---|*|    CL=operator register offset
;---|*|    DL=wave select
;---|*|
;---|*| Exit Condtions:
;---|*|    AH returns the volume for the operator
;---|*|
;   \*/

newloadoperator proc	near
;
; build a value for operator register 0x20 - AM+VIB+EG+KSR+MULT
;
	mov	ah,es:[si.op_AVEKM]	; get the multiplier
	mov	al,cl			; operator
	add	al,020h 		; reg base 20
        call    wrtIdxDta
;
; build a value for operator register 0x40 - KSL+TL
;
 if VELOCITY

;
; adjust the volume by both the velocity and channel volume
;
	mov	al,ds:[msvendor.dimsg.1]	; vel. is used for some volume
	add	al,ds:[msvendor.dichvol.di]	; add in the channel volume
	sbb	ah,ah				; saturate on overflow
	or	al,ah

        mov     ah,03Fh
	and	ah,es:[si.op_KSLTL]		; get the operator volume
	add	al,ah				; al=vel+chvol+TL
	sbb	ah,ah				; saturate on overflow
	or	al,ah

	cmp	al,3Fh				; saturate at 3Fh
	cmc					; set carry if over 3Fh
	sbb	ah,ah
	and	ax,3F3Fh
	or	ah,al				; final value in ah

	mov	al,0C0h 			; saturate at 3Fh
	and	al,es:[si.op_KSLTL]
	or	ah,al				; merge the top 2 bits

 else

  if 1
        %out Velocity Sensitivity is disable!
  endif
	mov	ah,es:[si.op_KSLTL]	; just use what is precalculated
 endif

	mov	al,40h
	add	al,cl
	call	wrtIdxDta
;
; build a value for operator register 0x60 - AR+DR
;
	mov	ah,es:[si.op_AD]
        mov     al,cl
        add     al,060h
        call    wrtIdxDta
;
; build a value for operator register 0x80 - SL+RR
;
	mov	ah,es:[si.op_SR]
        mov     al,cl
        add     al,080h
        call    wrtIdxDta
;
; output the value for the wave select
;
        mov     ah,dl
        mov     al,cl
        add     al,0E0h
        call    wrtIdxDta

	ret				; to be loaded later

newloadoperator endp

;   /*\
;---|*|--------------------====< xlatpatch >====-------------------------
;---|*|
;---|*| Translate the original format patch into one we can use internally.
;---|*|
;---|*| Entry Coinditions:
;---|*|    CL holds the patch #
;---|*|    CH holds the channel #
;---|*|    ES:SI point to the original, full sized patch structure
;---|*|
;---|*| Exit Condtions:
;---|*|    DX:AX point to the patch
;---|*|    BX saved
;---|*|
;   \*/

	public	xlatpatch
xlatpatch	proc	near
        push    di
;
; set the patch number now loaded for this channel. Then setup the patch
; frequency if this is a percussive channel.
;
	and	ch,0FH			; isolate the channel #
	cmp	ch,9			; if not channel 9, skip special cases
	jnz	xp_not_perc

        push    bx                      ; We will be saving any note frequency
        mov     bl,cl                   ; stored in the downloaded percussive
        and     bx,007Fh                ; patch

        mov     al,es:[si.opl2percVoice]; get the possible frequency
        cbw
        and     al,ah                   ; keep table entry if MSB=1
        not     ah
        and     ah,PERCUSSIVEKEY        ; select PERCUSSIVEKEY if MSB=0
        or      al,ah
        and     al,07Fh                 ; kill the MSB
	mov	cs:[holding.opfreq],al	; freq or 0

        pop     bx

        or      cl,80h                  ; force the patch # to be 128+
    ;
    xp_not_perc:
;
; setup to process the first operator
; Okay,  BX=voice, CL=operator, DL=wave sel, ES:SI point to the block
;
        push    si

	lea	si,[si.opl2op0] 	; point to the operator
	lea	di,cs:[holding.o1_AVEKM]
	call	xlatoperator		; load operator 0
;
; setup to process the second operator
; Again, BX=voice, CL=operator, DL=wave, ES:SI point to the block
;
	pop	si
        push    si

        lea     si,[si.opl2op1]         ; point to the operator
	lea	di,cs:[holding.o2_AVEKM]
	call	xlatoperator		; load operator 1

	pop	si			; restore SI
;
; build a value of: FB+C+WS1+WS2
;
;       v = ((op->feedBack & 0x7) << 1) + ((op->fm) ? 0x00 : 0x01);

	mov	al,es:[si.opl2op0.opl2feedBack]
	and	al,07H

        cmp     es:[si.opl2op0.opl2fm],1; CY = !fm
	adc	al,al
	shl	al,4

        mov     dl,es:[si.opl2wave0]    ; get the wave select
	mov	dh,es:[si.opl2wave1]
	shl	dh,2
	and	dx,0000110000000011b
	or	dl,dh
	or	al,dl

	mov	cs:[holding.opfb_c],al
;
; now, program the volumes to allow sound...
;
	lea	ax,cs:[holding] 	; do this for general purposes
	mov	dx,cs			; for other code to use this function

        pop     di
        ret

xlatpatch	endp

;
;   /*\
;---|*|----------------------====< xlatoperator >====-------------------------
;---|*|
;---|*| Translate an operator into the compressed format
;---|*|
;---|*| Entry Conditions:
;---|*|    DS points to the applications copy of the services structure.
;---|*|    ES:SI points to the operator patch.
;---|*|    BX=voice #
;---|*|    DL=wave select
;---|*|
;---|*| Exit Condtions:
;---|*|    AH returns the volume for the operator
;---|*|
;   \*/

xlatoperator	proc	near
;
; build a value for operator register 0x20 - AM+VIB+EG+KSR+MULT
;
;  v = (op->freqMult  & 0x0f)      +
;      ((op->am)      ? 0x80 : 0)  +
;      ((op->vib)     ? 0x40 : 0)  +
;      ((op->sustain) ? 0x20 : 0)  +
;      ((op->ksr)     ? 0x10 : 0);

        mov     al,es:[si.opl2freqMult]         ; get the multiplier
        and     ax,0Fh                          ; flush ah

        cmp     es:[si.opl2ksr],1
        cmc                                     ; CY if ksr
        rcr     ah,1

        cmp     es:[si.opl2sustain],1
        cmc                                     ; CY if sustain
        rcr     ah,1

        cmp     es:[si.opl2vib],1
        cmc                                     ; CY if vib
        rcr     ah,1

        cmp     es:[si.opl2am],1
        cmc                                     ; CY if am
        rcr     ah,1

        or      ah,al                           ; loadup ax
	mov	cs:[di.op_AVEKM],ah		; save in the structure
;
; build a value for operator register 0x40 - KSL+TL
;
;       v = ((op->ksl & 0x3) << 6) + (op->output & 0x3f);

        mov     ah,es:[si.opl2output]
        and     ah,03Fh                         ; keep just the valid bits

        mov     al,es:[si.opl2ksl]              ; get the ksl
        and     al,03h
        ror     al,2
        or      ah,al

	mov	cs:[di.op_KSLTL],ah		; save in the structure
;
; build a value for operator register 0x60 - AR+DR
;
;       v = ((op->attack & 0xf) << 4) + (op->decay & 0xf);

        mov     al,es:[si.opl2attack]
        shl     al,4
        mov     ah,es:[si.opl2decay]
        and     ah,0Fh
        add     ah,al

	mov	cs:[di.op_AD],ah		; save in the structure
;
; build a value for operator register 0x80 - SL+RR
;
;       v = ((op->sustLevel & 0xf) << 4) + (op->release & 0xf);
;
        mov     al,es:[si.opl2sustLevel]
        shl     al,4
        mov     ah,es:[si.opl2release]
        and     ah,0Fh
        add     ah,al

	mov	cs:[di.op_SR],ah		; save in the structure
	ret

xlatoperator	endp

        end

