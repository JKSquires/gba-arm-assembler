# Game Boy Advance ARM Assembler

A simple purpose-built ARM assembler targeting the Game Boy Advance.

---

## Usage

```sh
gbaaa <input ASM file> [output GBA file]
```

---

## Build Instructions

Compile the main C file.

Recommended way:

```sh
gcc -O2 -s main.c -o gbaaa
```

But if you really want to, you could just run:

```sh
gcc main.c
```

---

## Misc. Assembler Syntax

- *Labels* are delineated by a colon (`:`) at the end of the label's name. Label names are case insensitive alphanumeric text.
- *Comments* are any text after a semi-colon (`;`). They can be placed anywhere at the end of a line.
- *Instruction constants* must be prefixed with one of the following characters that determines the constant's base: `$` (hexadecimal; base-16), `#` (decimal; base-10), or `%` (binary; base-2).
- *Instruction operands* must be seperated by single commas (`,`) and can have any number of spaces or tabs between them.
- *Directive operands* do not necessarily need to be seperated by commas, but must be seperated by at least one non-alphanumeric character (e.g. comma, space, tab, underscore, etc.) with the exception of the following characters: `$`, `#`, `%`, and `;`.

---

## Supported Assembler Directives

Name | Directive | Example Usage
---- | --------- | -------------
Define bytes | `@b <byte list>` | `@b $10, #255`
Define halfwords | `@h <halfword list>` | `@h %10000000011`
Define words | `@w <word list>` | `@w $05000200,$4000130, %110000000000000000000000010`

## Supported Instructions

Name | Instruction | Example Usage
---- | ----------- | -------------
Add with Carry | `adc{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `adc r0, r0, r1` / `adc r3, r12, #127` / `adc r2, r3, r4, asr r5`
Add | `add{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `add r0, r0, r1` / `add r3, r12, #127` / `add r2, r3, r4, asr r5`
Bitwise And | `and{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `and r0, r0, r1` / `and r3, r12, #127` / `and r2, r3, r4, asr r5`
Branch | `b{<cond>} <label>` | `b start`
Bitwise Bit Clear | `bic{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `bic r0, r0, r1` / `bic r3, r12, #127` / `bic r2, r3, r4, asr r5`
Branch with Link | `bl{<cond>} <label>` | `bl div`
Branch and Exchange | `bx{<cond>} <Rm>` | `bx r14`
Compare Negative | `cmn{<cond>}{s} <Rn>, <oprnd2> {, <shift>}` | `cmn r0, r2` / `cmn r8,$6000000` / `cmneq r5, r3, lsr #3`
Compare | `cmp{<cond>}{s} <Rn>, <oprnd2> {, <shift>}` | `cmp r0, r2` / `cmp r8,$6000000` / `cmpeq r5, r3, lsr #3`
Bitwise Exclusive Or | `eor{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `eor r0, r0, r1` / `eor r3, r12, #127` / `eor r2, r3, r4, asr r5`
Load Multiple | `ldm{cond}{<da/db/ea/ed/fa/fd/ia/ib>} <Rn>{!}, <register_list>` | `ldm r0, {r3, r4}` / `ldmfd r13!, {r1-r5, r7, r9-r11}` / `ldmeqia r2, {r4-r6}`
Load Register (Immediate) | `ldr{cond}{b} <Rt>, [<Rn>{, <imm12>}]{!}` / `ldr{cond}{b} <Rt>, [<Rn>], <imm12>` | `ldr r1, [r2]` / `ldrb r3, [r4,#-6]` / `ldrneb r2, [r3], $A` / `ldr r5, [r2, %10011]!`
Load Register (Literal) | `ldr{<cond>}{b} <Rt>, <label>` | `ldr r2, num1`
Load Register (Register) | `ldr{cond}{b} <Rt>, [<Rn>, +/-<Rm>{, <shift>}]{!}` / `ldr{cond}{b} <Rt>, [<Rn>], +/-<Rm>{, <shift>}` | `ldr r6, [r2, r3]` / `ldreq r7, [r1, -r2]!` / `ldrb r4, [r3, r4, ror #2]` / `ldreqb r2, [r1], r5, lsl #1`
Load Register Unprivileged (Immediate) | `ldr{cond}{b}t <Rt>, [<Rn>]{, <imm12>}` | `ldrbt r2, [r6]` / `ldreqt r7, [r1], #6`
Load Register Unprivileged (Register) | `ldr{cond}{b}t <Rt>, [<Rn>], +/-<Rm>{, <shift>}` | `ldrt r4, [r3], r5` / `ldrbt r1, [r6], -r2, lsr #2`
Load Halfword / Signed Halfword / Signed Byte (Immediate) | `ldr{cond}<h/sh/sb> <Rt>, [<Rn>{, #+/-<imm8>}]{!}` / `ldr{cond}<h/sh/sb> <Rt>, [<Rn>], #+/-<imm8>` | `ldrsh r1, [r2]` / `ldrh r3, [r4,#-6]` / `ldrneh r2, [r3], $A` / `ldrsb r5, [r2, %10011]!`
Load Halfword / Signed Halfword / Signed Byte (Literal) | `ldr{<cond>}<h/sh/sb> <Rt>, <label>` | `ldrh r2, num1`
Load Halfword / Signed Halfword / Signed Byte (Register) | `ldr{cond}<h/sh/sb> <Rt>, [<Rn>, +/-<Rm>]{!}` / `ldr{cond}<h/sh/sb> <Rt>, [<Rn>], +/-<Rm>` | `ldrh r6, [r2, r3]` / `ldreqh r7, [r1, -r2]!` / `ldrsh r0, [r4], r2`
Multiply Accumulate | `mla{<cond>}{s} <Rd>, <Rn>, <Rm>, <Ra>` | `mla r0, r0, r1, r2`
Move | `mov{<cond>}{s} <Rd>, <oprnd2> {, <shift>}` | `mov r0, r2` / `mov r8,$6000000` / `moveqs r5, r3, lsr #3`
Multiply | `mul{<cond>}{s} <Rd>, <Rn>, <Rm>` | `mul r0, r0, r1`
Move Not | `mvn{<cond>}{s} <Rd>, <oprnd2> {, <shift>}` | `mvn r0, r2` / `mvn r8,$6000000` / `mvneqs r5, r3, lsr #3`
Bitwise Or | `orr{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `orr r0, r0, r1` / `orr r3, r12, #127` / `orr r2, r3, r4, asr r5`
Reverse Subtract | `rsb{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `rsb r0, r0, r1` / `rsb r3, r12, #127` / `rsb r2, r3, r4, asr r5`
Reverse Subtract with Carry | `rsc{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `rsc r0, r0, r1` / `rsc r3, r12, #127` / `rsc r2, r3, r4, asr r5`
Subtract with Carry | `sbc{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `sbc r0, r0, r1` / `sbc r3, r12, #127` / `sbc r2, r3, r4, asr r5`
Store Multiple | `stm{cond}{<da/db/ea/ed/fa/fd/ia/ib>} <Rn>{!}, <register_list>` | `stm r0, {r3, r4}` / `stmfd r13!, {r1-r5, r7, r9-r11}` / `stmeqia r2, {r4-r6}`
Store Register (Immediate) | `str{cond}{b} <Rt>, [<Rn>{, <imm12>}]{!}` / `str{cond}{b} <Rt>, [<Rn>], <imm12>` | `str r1, [r2]` / `strb r3, [r4,#-6]` / `strneb r2, [r3], $A` / `str r5, [r2, %10011]!`
Store Register (Register) | `str{cond}{b} <Rt>, [<Rn>, +/-<Rm>{, <shift>}]{!}` / `str{cond}{b} <Rt>, [<Rn>], +/-<Rm>{, <shift>}` | `str r6, [r2, r3]` / `streq r7, [r1, -r2]!` / `strb r4, [r3, r4, ror #2]` / `streqb r2, [r1], r5, lsl #1`
Store Register Unprivileged (Immediate) | `str{cond}{b}t <Rt>, [<Rn>]{, <imm12>}` | `strbt r2, [r6]` / `streqt r7, [r1], #6`
Store Register Unprivileged (Register) | `str{cond}{b}t <Rt>, [<Rn>], +/-<Rm>{, <shift>}` | `strt r4, [r3], r5` / `strbt r1, [r6], -r2, lsr #2`
Store Halfword / Signed Halfword / Signed Byte (Immediate) | `str{cond}<h/sh/sb> <Rt>, [<Rn>{, #+/-<imm8>}]{!}` / `str{cond}<h/sh/sb> <Rt>, [<Rn>], #+/-<imm8>` | `strsh r1, [r2]` / `strh r3, [r4,#-6]` / `strneh r2, [r3], $A` / `strsb r5, [r2, %10011]!`
Store Halfword / Signed Halfword / Signed Byte (Literal) | `str{<cond>}<h/sh/sb> <Rt>, <label>` | `strh r2, num1`
Store Halfword / Signed Halfword / Signed Byte (Register) | `str{cond}<h/sh/sb> <Rt>, [<Rn>, +/-<Rm>]{!}` / `str{cond}<h/sh/sb> <Rt>, [<Rn>], +/-<Rm>` | `strh r6, [r2, r3]` / `streqh r7, [r1, -r2]!` / `strsh r0, [r4], r2`
Signed Multiply Accumulate Long | `umlal{<cond>}{s} <RdLo>, <RdHi>, <Rn>, <Rm>` | `umlal r0, r1, r5, r8`
Signed Multiply Long | `umull{<cond>}{s} <RdLo>, <RdHi>, <Rn>, <Rm>` | `umull r0, r1, r5, r8`
Subtract | `sub{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `sub r0, r0, r1` / `sub r3, r12, #127` / `sub r2, r3, r4, asr r5`
Software Interrupt | `swi{<cond>} <imm24>` | `swi $250000`
Swap | `swp{<cond>}{b} <Rt>, <Rt2>, [<Rn>]` | `swp r4, r2, [r7]` / `swpb r1, r2, [r4]` / `swpneb r9, r6, [r7]`
Test Equivalence | `teq{<cond>}{s} <Rn>, <oprnd2> {, <shift>}` | `teq r0, r2` / `teq r8,$6000000` / `teqeq r5, r3, lsr #3`
Test | `tst{<cond>}{s} <Rn>, <oprnd2> {, <shift>}` | `tst r0, r2` / `tst r8,$6000000` / `tsteq r5, r3, lsr #3`
Unsigned Multiply Accumulate Long | `umlal{<cond>}{s} <RdLo>, <RdHi>, <Rn>, <Rm>` | `umlal r0, r1, r5, r8`
Unsigned Multiply Long | `umull{<cond>}{s} <RdLo>, <RdHi>, <Rn>, <Rm>` | `umull r0, r1, r5, r8`

### Pseudo-Instructions

Name | Instruction | Example Usage
---- | ----------- | -------------
Arithmetic Shift Right | `asr{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `asr r0, r0, #4` / `asrne r2,r4,r3`
Logical Shift Left | `lsl{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `lsl r0, r0, #4` / `lslne r2,r4,r3`
Logical Shift Right | `lsr{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `lsr r0, r0, #4` / `lsrne r2,r4,r3`
No Operation | `nop` | `nop`
Pop Multiple Registers | `pop{<cond>} <register_list>` | `pop {r0-r2, r4, r14}`
Push Multiple Registers | `push{<cond>} <register_list>` | `push {r0-r2, r4, r14}`
Rotate Right | `ror{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `ror r0, r0, #4` / `rorne r2,r4,r3`
Rotate Right with Extend | `rrx{<cond>}{s} <Rd>, <Rm>` | `rrx r2, r1`
Supervisor Call | `svc{<cond>} <imm24>` | `svc $250000`
Generate Undefined Instruction | `und{<cond>} {#expr}` | `und` / `und #1234`

