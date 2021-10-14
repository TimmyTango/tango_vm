    mov r0, #0              ; r0 = 0 (not really needed)
    mov xl, <data           ; low-byte of x 
    mov xh, >data           ; high-byte of x
loop:
    jsr push_data           ; call push_data subroutine
    inc r0                  ; increment r0
    cmp r0, data_size       ; compare r0 to data_size (#8)
    bne loop                ; repeat loop if r0 != 8

    mov r1, $F000           ; check reading from ROM
    mov $FF00, #$7F         ; check writing to ROM
    mov r2, $FF00           ; should have no effect
    end                     ; end of program

push_data:
    psh x           ; push value x points to onto stack
    inc xl          ; increment low-byte of x
    adc xh, 0       ; increment high-byte of x if carry was set
    ret             ; end of subroutine, jump back to where we were

data:
    .byte $00 $10 $20 $30 $40 $50 $60 $70

data_size:
    .byte 8