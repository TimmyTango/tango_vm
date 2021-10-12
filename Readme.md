# VM Spec

This is a toy implementation of a made-up CPU ISA, VM to run it, and a simple assembler.
It's written for fun and to practice lower-level programming.

## Registers

### 8-bit registers

- r0 - r7: general purpose registers
- ST ($8): status register (bits: 7-0 = xxxxxCNZ)
    Z: last operation returned 0
    N: last operation returned negative byte (bit 7 set to 1)
    C: last operation set carry flag
- SL/SH ($9, $A): low/high byte of SP
- XL/XH ($B, $C): low/high byte of X
- YL/YH ($D, $E): low/high byte of Y

### 16-bit registers

- PC: program counter
- SP: stack pointer
- X: general purpose (useful for pointers to memory)
- Y: general purpose (useful for pointers to memory)

### Instruction set

Misc/one-off instructions

- $00: nop - no-op
- $10: jmp - 16-bit jump
- $20: inc - increment register
- $30: dec - decrement register
- $40: clc - clear carry
- $50: sec - set carry
- $60: not - bitwise not on reg
- $70: jsr - jump to sub-routine
- $80: ret - return from sub-routine

Branching

- $01: beq - branch if Z flag = 1
- $11: bne - branch if Z flag = 0
- $21: blt - branch if C flag = 1
- $31: ble - branch if C flag = 1 or Z flag = 1
- $41: bgt - branch if C flag = 0 and Z flag = 0
- $51: bge - branch if C flag = 0 or Z flag = 1

Move

- $02: reg = reg
- $12: reg = mem
- $22: reg = immediate
- $32: reg = indirect
- $42: mem = reg
- $52: mem = mem
- $62: mem = immediate
- $72: mem = indirect
- $82: indirect = reg
- $92: indirect = mem
- $a2: indirect = immediate
- $b2: indirect = indirect

Add

- $03: reg += reg (```add r0, r1           ; r0 += r1```)
- $13: reg += mem (```add r0, $0200        ; r0 += mem[$0200]```)
- $23: reg += immediate (```add r0, #$a1   ; r0 += $a1```)
- $33: reg += indirect (```add r0, [$0200] ; r0 += mem[mem[$0200]]```)
- $43: adc version of $03
- $53: adc version of $13
- $63: adc version of $23
- $73: adc version of $33

Sub

- $04: reg -= reg (```sub r0, r1             ; r0 -= r1```)
- $14: reg -= mem (```sub r0, $0200          ; r0 -= mem[$0200]```)
- $24: reg -= immediate (```sub r0, #$a1     ; r0 -= $a1```)
- $34: reg -= indirect (```sub r0, [$0200]   ; r0 -= mem[mem[$0200]]```)
- $44: sbb version of $04
- $54: sbb version of $14
- $64: sbb version of $24
- $74: sbb version of $34

Compare

- $05: cmp reg to reg
- $15: cmp reg to mem
- $25: cmp reg to immediate
- $35: cmp reg to indirect

And/Or

- $07: and reg &= reg
- $17: and reg &= mem
- $27: and reg &= immediate
- $37: and reg &= indirect
- $47: or reg |= reg
- $57: or reg |= mem
- $67: or reg |= immediate
- $77: or reg |= indirect

Stack Manipulation

- $08: push reg
- $18: push mem
- $28: push immediate
- $38: push indirect
- $48: pop into reg
- $58: pop into mem
- $78: pop into indirect
