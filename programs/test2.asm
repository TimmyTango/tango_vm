    mov r0, #0
start_loop:
    cmp r0, #10
    beq the_end
    add r0, #1
    jmp start_loop
the_end
    end



    mov r0, #0
    .while r0 < 10
        add r0, #1
    .endwhile
    end
 