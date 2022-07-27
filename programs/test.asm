    .equ ROW r1
    .equ COL r2
    .equ LAST_TILE_VAL r3
    .equ IS_LAST_TILE r4
    .equ PAD r5

    .equ PAD1 $FCB0
    .equ B_UP #$01
    .equ B_DOWN #$02
    .equ B_LEFT #$04
    .equ B_RIGHT #$08
    .equ B_A #$10
    .equ B_B #$20
    .equ B_SELECT #$40
    .equ B_START #$80

    .equ SPRITE1 $FCB2
    .equ SPRITE1_X $FCB3
    .equ SPRITE1_Y $FCB4

    .org $200
    mov xl, #$00
    mov xh, #$F8
    mov ROW, #0
    mov COL, #0
    mov LAST_TILE_VAL, #1
    mov SPRITE1, #63

draw_map:
    jsr draw_tile
    jsr next_tile_value
    jsr advance_col
    jsr check_if_last_tile
    cmp IS_LAST_TILE, #1
    beq never_ending_loop
    jmp draw_map

draw_tile:
    mov x, LAST_TILE_VAL
    inc xl
    adc xh, #0
    ret

advance_col:
    inc COL
    cmp COL, #32
    bne same_row
    inc ROW
    mov COL, #0
    jsr next_tile_value
same_row:
    ret

next_tile_value:
    inc LAST_TILE_VAL
    cmp LAST_TILE_VAL, #2
    ble skip_last_tile_reset
    mov LAST_TILE_VAL, #1
skip_last_tile_reset:
    ret

check_if_last_tile:
    cmp ROW, #18
    bne is_not_last_tile
    mov IS_LAST_TILE, #1
    ret
is_not_last_tile:
    mov IS_LAST_TILE, #0
    ret

never_ending_loop:
    mov r0, B_DOWN
    jsr check_input
    bne after_down
    jsr move_down
after_down:
    mov r0, B_UP
    jsr check_input
    bne after_up
    jsr move_up
after_up:
    mov r0, B_LEFT
    jsr check_input
    bne after_left
    jsr move_left
after_left:
    mov r0, B_RIGHT
    jsr check_input
    bne after_right
    jsr move_right
after_right:
    mov r0, #0
    jsr spin_tires
    jmp never_ending_loop

check_input:
    mov PAD, PAD1
    and PAD, r0
    cmp PAD, r0
    ret

move_down:
    mov r0, SPRITE1_Y
    inc r0
    jsr fix_y_coord
    mov SPRITE1_Y, r0
    ret

move_up:
    mov r0, SPRITE1_Y
    dec r0
    blt wrap_to_144
    mov SPRITE1_Y, r0
    ret

fix_y_coord:
    cmp r0, #144
    blt skip_fix_y_coord
    sub r0, #144
skip_fix_y_coord:
    ret

move_left:
    mov r0, SPRITE1_X
    dec r0
    mov SPRITE1_X, r0
    ret

move_right:
    mov r0, SPRITE1_X
    inc r0
    mov SPRITE1_X, r0
    ret

spin_tires:
    dec r0
    cmp r0, #0
    bne spin_tires
    ret