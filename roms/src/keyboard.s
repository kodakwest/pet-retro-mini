; PET keyboard matrix scan and PETSCII lookup helpers.
; This file is included by kernal.s.

KEY_NONE = $00

keyboard_scan:
    ldx #$00
scan_row:
    lda row_masks,x
    sta PIA1_PORT_A
    lda PIA1_PORT_B
    cmp #$ff
    bne key_found
    inx
    cpx #$0a
    bne scan_row
    lda #KEY_NONE
    rts

key_found:
    ldy #$00
find_col:
    lsr
    bcc map_key
    iny
    cpy #$08
    bne find_col
    lda #KEY_NONE
    rts

map_key:
    txa
    asl
    asl
    asl
    sta tmp_key
    tya
    clc
    adc tmp_key
    tax
    lda petscii_table,x
    rts

row_masks:
    .byte $fe,$fd,$fb,$f7,$ef,$df,$bf,$7f,$00,$00

; Conservative alphanumeric map. Unknown or modifier positions return 0.
petscii_table:
    .byte "12345678"
    .byte "QWERTYUI"
    .byte "ASDFGHJK"
    .byte "ZXCVBNM,"
    .byte "90-=", $0d, $14, $20, $00
    .byte "OP[]", $91, $11, $1d, $9d
    .byte "L;:", $00, $00, $00, $00, $00
    .byte "./", $00, $00, $00, $00, $00, $00
    .byte $00,$00,$00,$00,$00,$00,$00,$00
    .byte $00,$00,$00,$00,$00,$00,$00,$00
