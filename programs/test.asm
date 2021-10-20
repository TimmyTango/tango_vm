    mov xl, #$00
    mov xh, #$F0

    mov yl, <tile1
    mov yh, >tile1
    jsr draw_tile

    mov xl, #$04
    mov xh, #$F0

    mov yl, <tile2
    mov yh, >tile2
    jsr draw_tile

    jmp never_ending_loop

draw_tile:
    psh r0
    psh yl
    psh yh
    mov r0, #8
draw_tile_loop:
    jsr draw_row
    add xl, #28
    adc xh, #0
    add yl, #4
    adc yh, #0
    dec r0
    bne draw_tile_loop
    pop r0
    pop yl
    pop yh
    ret

draw_row:
    psh yl
    psh yh
    psh r0
    mov r0, #4

draw_row_loop:
    mov x, y
    inc yl
    adc yh, #0
    inc xl
    adc xh, #0
    dec r0
    bne draw_row_loop

    pop r0
    pop yh
    pop yl
    ret

never_ending_loop:
    jmp never_ending_loop

tile1:
    .byte $56 $56 $56 $56 
    .byte $65 $65 $65 $65
    .byte $56 $56 $56 $56 
    .byte $65 $65 $65 $65
    .byte $56 $56 $56 $56 
    .byte $65 $65 $65 $65
    .byte $56 $56 $56 $56 
    .byte $65 $65 $65 $65

tile2:
    .byte $34 $34 $34 $34
    .byte $43 $43 $43 $43
    .byte $34 $34 $34 $34
    .byte $43 $43 $43 $43
    .byte $34 $34 $34 $34
    .byte $43 $43 $43 $43
    .byte $34 $34 $34 $34
    .byte $43 $43 $43 $43
