    mov r0, #0              ; r0 = 0 (not really needed)
    mov xl, <data           ; low-byte of x 
    mov xh, >data           ; high-byte of x
loop:
    jsr push_data           ; call push_data subroutine
    inc r0                  ; increment r0
    cmp r0, data_size       ; compare r0 to data_size (#8)
    bne loop                ; repeat loop if r0 != 8
    end                     ; end of program

push_data:
    pop yl          ; store low-byte of return address
    pop yh          ; store high-byte of return address

    psh x           ; push value x points to onto stack
    inc xl          ; increment low-byte of x
    adc xh, 0       ; increment high-byte of x if carry was set

    psh yl          ; push low-byte of return address
    psh yh          ; push high-byte of return address
    ret             ; end of subroutine, jump back to where we were

data:
    .byte $00 $10 $20 $30 $40 $50 $60 $70

data_size:
    .byte 8