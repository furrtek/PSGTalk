; PSGTalk playback example for Sega Master System
; furrtek 2015

.memorymap
defaultslot 0
slotsize $8000
slot 0 $0000
.endme

.rombankmap
bankstotal 1
banksize $8000
banks 1
.endro

.computesmschecksum

.define WORKRAM $C000
.define VBL_COUNTER WORKRAM

.bank 0 slot 0
.org $0000
	di
	im 1
	jp main

.org $0038
	push af
	in a,($BF)		; Ack VBI
	push bc
	push de
	;push hl
	push ix

	call VBI

	pop ix
	;pop hl
	pop de
	pop bc
	pop af
	ei
	ret

.org $0066
    	retn

VBI:
	ld e,0  		; Channel counter
-:
	ld a,(hl)		; Volume, and frequency MSBs
	inc hl
	ld b,a
	srl a
	srl a
	xor $0F
	or %10010000		; Channel 1 volume
	or e
	out ($7F),a
	ld a,b
	and 3
	sla a
	sla a
	sla a
	sla a
	ld b,a
	ld a,(hl)		; Frequency LSBs
	inc hl
	ld c,a
	srl a
	srl a
	srl a
	srl a
	or b
	ld b,a
	ld a,c
	and $0F
	or %10000000		; Channel 1 frequency
	or e
	out ($7F),a
	ld a,b
	out ($7F),a
	
	ld a,e
	cp 2<<5
	ret z
	add a,1<<5
	ld e,a
	jr -

main:
	ld sp, $DFF0

	ld a,%00010110		; RLI enabled
	out ($BF),a
	ld a,$80
	out ($BF),a

	ld a,(192/4)-1		; RLI 4 times per frame
	out ($BF),a
	ld a,$8A
	out ($BF),a

	ld a,%11111111		; Shut channel 4 up
	out ($7F),a

	ld hl,talkdata

	ei

lp:
    jp lp

talkdata:
.incbin "sonic.bin"
talkdata_end:

.org $7FF0
.db "TMR SEGA"
