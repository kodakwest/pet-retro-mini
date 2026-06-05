; PET Retro Mini open screen editor.
; This ROM mirrors editor routines also present in the minimal KERNAL.

.setcpu "6502"

SCREEN      = $8000
COLS        = 40
ROWS        = 25
cursor_x    = $00f0
cursor_y    = $00f1
screen_ptr  = $00f2

.segment "CODE"

editor_init:
    jsr editor_clear
    rts

editor_clear:
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

editor_home:
    lda #$00
    sta cursor_x
    sta cursor_y
    rts

editor_scroll:
    lda #$18
    sta cursor_y
    lda #$00
    sta cursor_x
    rts

editor_line_ready:
    lda #$01
    sta $027f
    rts

editor_get_line:
    ldx #$00
copy_loop:
    lda $0200,x
    beq copy_done
    inx
    cpx #$50
    bne copy_loop
copy_done:
    rts
