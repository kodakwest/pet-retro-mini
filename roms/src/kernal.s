; PET Retro Mini open KERNAL.
; Minimal PET-compatible reset, CHROUT, CHRIN, IRQ, and PLOT entry points.

.setcpu "6502"

SCREEN      = $8000
COLS        = 40
ROWS        = 25
PIA1_PORT_A = $E810
PIA1_DDRA   = $E811
PIA1_PORT_B = $E812
PIA1_DDRB   = $E813
PIA2_PORT_A = $E820
PIA2_DDRA   = $E821
PIA2_PORT_B = $E822
PIA2_DDRB   = $E823
VIA_T1CL    = $E844
VIA_T1CH    = $E845
VIA_ACR     = $E84B
VIA_IER     = $E84E
VIA_IFR     = $E84D
CRTC_ADDR   = $E880
CRTC_DATA   = $E881
BASIC_COLD  = $C000

cursor_x    = $00f0
cursor_y    = $00f1
screen_ptr  = $00f2
tmp_key     = $00f4
irqlo       = $0200
irqhi       = $0201

.segment "CODE"

reset:
    sei
    cld
    ldx #$ff
    txs
    inx
    stx cursor_x
    stx cursor_y
    stx irqlo
    stx irqhi

    lda #$ff
    sta PIA1_DDRA
    lda #$00
    sta PIA1_DDRB
    lda #$ff
    sta PIA2_DDRA
    sta PIA2_DDRB

    lda #$40
    sta VIA_ACR
    lda #$7f
    sta VIA_IER
    lda #$ff
    sta VIA_T1CL
    lda #$40
    sta VIA_T1CH

    jsr init_crtc_40
    jsr clear_screen
    cli
    jmp BASIC_COLD

init_crtc_40:
    ldx #$00
crtc_loop:
    cpx #$0e
    beq crtc_done
    stx CRTC_ADDR
    lda crtc_40,x
    sta CRTC_DATA
    inx
    bne crtc_loop
crtc_done:
    rts

clear_screen:
    lda #<SCREEN
    sta screen_ptr
    lda #>SCREEN
    sta screen_ptr+1
    ldx #$04
clear_page:
    ldy #$00
    lda #$20
clear_byte:
    sta (screen_ptr),y
    iny
    bne clear_byte
    inc screen_ptr+1
    dex
    bne clear_page
    lda #$00
    sta cursor_x
    sta cursor_y
    rts

chrout:
    cmp #$0d
    beq new_line
    cmp #$0a
    beq line_down
    cmp #$13
    beq home
    cmp #$93
    beq clear_screen
    cmp #$1d
    beq cursor_right
    cmp #$9d
    beq cursor_left
    cmp #$91
    beq cursor_up
    cmp #$11
    beq line_down
    jsr calc_screen_ptr
    ldy #$00
    sta (screen_ptr),y
cursor_right:
    inc cursor_x
    lda cursor_x
    cmp #COLS
    bcc chrout_done
new_line:
    lda #$00
    sta cursor_x
line_down:
    inc cursor_y
    lda cursor_y
    cmp #ROWS
    bcc chrout_done
    jsr scroll
chrout_done:
    clc
    rts

cursor_left:
    lda cursor_x
    beq chrout_done
    dec cursor_x
    rts

cursor_up:
    lda cursor_y
    beq chrout_done
    dec cursor_y
    rts

home:
    lda #$00
    sta cursor_x
    sta cursor_y
    rts

calc_screen_ptr:
    lda #<SCREEN
    sta screen_ptr
    lda #>SCREEN
    sta screen_ptr+1
    ldx cursor_y
row_loop:
    cpx #$00
    beq add_x
    clc
    lda screen_ptr
    adc #COLS
    sta screen_ptr
    lda screen_ptr+1
    adc #$00
    sta screen_ptr+1
    dex
    bne row_loop
add_x:
    clc
    lda screen_ptr
    adc cursor_x
    sta screen_ptr
    bcc ptr_done
    inc screen_ptr+1
ptr_done:
    rts

scroll:
    lda #$01
    sta cursor_y
    jsr calc_screen_ptr
    lda #<SCREEN
    sta irqlo
    lda #>SCREEN
    sta irqhi
    ldx #$18
scroll_row:
    ldy #$00
scroll_col:
    lda (screen_ptr),y
    sta (irqlo),y
    iny
    cpy #COLS
    bne scroll_col
    clc
    lda screen_ptr
    adc #COLS
    sta screen_ptr
    lda screen_ptr+1
    adc #$00
    sta screen_ptr+1
    clc
    lda irqlo
    adc #COLS
    sta irqlo
    lda irqhi
    adc #$00
    sta irqhi
    dex
    bne scroll_row
    lda #$18
    sta cursor_y
    lda #$00
    sta cursor_x
    jsr calc_screen_ptr
    ldy #$00
    lda #$20
clear_bottom:
    sta (screen_ptr),y
    iny
    cpy #COLS
    bne clear_bottom
    rts

chrin:
    jsr keyboard_scan
    rts

plot:
    bcs plot_get
    stx cursor_y
    sty cursor_x
    rts
plot_get:
    ldx cursor_y
    ldy cursor_x
    rts

irq:
    lda VIA_IFR
    and #$40
    beq irq_exit
    lda VIA_T1CL
    jsr keyboard_scan
irq_exit:
    lda irqlo
    ora irqhi
    beq irq_rti
    jmp (irqlo)
irq_rti:
    rti

nmi:
    rti

crtc_40:
    .byte 49,40,45,15,32,8,25,29,0,9,0,0,0,0

.include "keyboard.s"

.segment "CHROUT_JUMP"
    jmp chrout

.segment "CHRIN_JUMP"
    jmp chrin

.segment "PLOT_JUMP"
    jmp plot

.segment "VECTORS"
    .word nmi
    .word reset
    .word irq
