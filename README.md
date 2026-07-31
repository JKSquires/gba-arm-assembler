# Game Boy Advance ARM Assembler

A simple purpose-built ARM assembler targeting the Game Boy Advance.

---

## Usage

```sh
gbaaa [-i] [-v] <input ASM file> [output GBA file]
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
Add with Carry | `adc{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `adc r0, r0, r1`<br>`adc r3, r12, #127`<br>`adc r2, r3, r4, asr r5`
Add | `add{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `add r0, r0, r1`<br>`add r3, r12, #127`<br>`add r2, r3, r4, asr r5`
Bitwise And | `and{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `and r0, r0, r1`<br>`and r3, r12, #127`<br>`and r2, r3, r4, asr r5`
Branch | `b{<cond>} <label>` | `b start`
Bitwise Bit Clear | `bic{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `bic r0, r0, r1`<br>`bic r3, r12, #127`<br>`bic r2, r3, r4, asr r5`
Branch with Link | `bl{<cond>} <label>` | `bl div`
Branch and Exchange | `bx{<cond>} <Rm>` | `bx r14`
Compare Negative | `cmn{<cond>}{s} <Rn>, <oprnd2> {, <shift>}` | `cmn r0, r2`<br>`cmn r8,$6000000`<br>`cmneq r5, r3, lsr #3`
Compare | `cmp{<cond>}{s} <Rn>, <oprnd2> {, <shift>}` | `cmp r0, r2`<br>`cmp r8,$6000000`<br>`cmpeq r5, r3, lsr #3`
Bitwise Exclusive Or | `eor{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `eor r0, r0, r1`<br>`eor r3, r12, #127`<br>`eor r2, r3, r4, asr r5`
Load Multiple | `ldm{cond}{<da/db/ea/ed/fa/fd/ia/ib>} <Rn>{!}, <register_list>` | `ldm r0, {r3, r4}`<br>`ldmfd r13!, {r1-r5, r7, r9-r11}`<br>`ldmeqia r2, {r4-r6}`
Load Register (Immediate) | `ldr{cond}{b} <Rt>, [<Rn>{, <imm12>}]{!}`<br>`ldr{cond}{b} <Rt>, [<Rn>], <imm12>` | `ldr r1, [r2]`<br>`ldrb r3, [r4,#-6]`<br>`ldrneb r2, [r3], $A`<br>`ldr r5, [r2, %10011]!`
Load Register (Literal) | `ldr{<cond>}{b} <Rt>, <label>` | `ldr r2, num1`
Load Register (Register) | `ldr{cond}{b} <Rt>, [<Rn>, +/-<Rm>{, <shift>}]{!}`<br>`ldr{cond}{b} <Rt>, [<Rn>], +/-<Rm>{, <shift>}` | `ldr r6, [r2, r3]`<br>`ldreq r7, [r1, -r2]!`<br>`ldrb r4, [r3, r4, ror #2]`<br>`ldreqb r2, [r1], r5, lsl #1`
Load Register Unprivileged (Immediate) | `ldr{cond}{b}t <Rt>, [<Rn>]{, <imm12>}` | `ldrbt r2, [r6]`<br>`ldreqt r7, [r1], #6`
Load Register Unprivileged (Register) | `ldr{cond}{b}t <Rt>, [<Rn>], +/-<Rm>{, <shift>}` | `ldrt r4, [r3], r5`<br>`ldrbt r1, [r6], -r2, lsr #2`
Load Halfword / Signed Halfword / Signed Byte (Immediate) | `ldr{cond}<h/sh/sb> <Rt>, [<Rn>{, #+/-<imm8>}]{!}`<br>`ldr{cond}<h/sh/sb> <Rt>, [<Rn>], #+/-<imm8>` | `ldrsh r1, [r2]`<br>`ldrh r3, [r4,#-6]`<br>`ldrneh r2, [r3], $A`<br>`ldrsb r5, [r2, %10011]!`
Load Halfword / Signed Halfword / Signed Byte (Literal) | `ldr{<cond>}<h/sh/sb> <Rt>, <label>` | `ldrh r2, num1`
Load Halfword / Signed Halfword / Signed Byte (Register) | `ldr{cond}<h/sh/sb> <Rt>, [<Rn>, +/-<Rm>]{!}`<br>`ldr{cond}<h/sh/sb> <Rt>, [<Rn>], +/-<Rm>` | `ldrh r6, [r2, r3]`<br>`ldreqh r7, [r1, -r2]!`<br>`ldrsh r0, [r4], r2`
Multiply Accumulate | `mla{<cond>}{s} <Rd>, <Rn>, <Rm>, <Ra>` | `mla r0, r0, r1, r2`
Move | `mov{<cond>}{s} <Rd>, <oprnd2> {, <shift>}` | `mov r0, r2`<br>`mov r8,$6000000`<br>`moveqs r5, r3, lsr #3`
Move to Register from Special Register | `mrs{<cond>} <Rd>, <PSR>` | `mrs r3, CPSR`<br>`mrseq r6, SPSR`
Move to Special Register (Immediate) | `msr{<cond>} <PSR>{_<field_mask>}, <imm12>` | `msr CPSR, #6`<br>`msrne CPSR_xf, %100111101101000`<br>`msr SPSR_cxsf, $EF`
Move to Special Register (Register) | `msr{<cond>} <PSR>{_<field_mask>}, <Rn>` | `msr SPSR, r2`<br>`msrne CPSR_cxs, r7`<br>`msr SPSR_cx, r0`
Multiply | `mul{<cond>}{s} <Rd>, <Rn>, <Rm>` | `mul r0, r0, r1`
Move Not | `mvn{<cond>}{s} <Rd>, <oprnd2> {, <shift>}` | `mvn r0, r2`<br>`mvn r8,$6000000`<br>`mvneqs r5, r3, lsr #3`
Bitwise Or | `orr{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `orr r0, r0, r1`<br>`orr r3, r12, #127`<br>`orr r2, r3, r4, asr r5`
Reverse Subtract | `rsb{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `rsb r0, r0, r1`<br>`rsb r3, r12, #127`<br>`rsb r2, r3, r4, asr r5`
Reverse Subtract with Carry | `rsc{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `rsc r0, r0, r1`<br>`rsc r3, r12, #127`<br>`rsc r2, r3, r4, asr r5`
Subtract with Carry | `sbc{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `sbc r0, r0, r1`<br>`sbc r3, r12, #127`<br>`sbc r2, r3, r4, asr r5`
Store Multiple | `stm{cond}{<da/db/ea/ed/fa/fd/ia/ib>} <Rn>{!}, <register_list>` | `stm r0, {r3, r4}`<br>`stmfd r13!, {r1-r5, r7, r9-r11}`<br>`stmeqia r2, {r4-r6}`
Store Register (Immediate) | `str{cond}{b} <Rt>, [<Rn>{, <imm12>}]{!}`<br>`str{cond}{b} <Rt>, [<Rn>], <imm12>` | `str r1, [r2]`<br>`strb r3, [r4,#-6]`<br>`strneb r2, [r3], $A`<br>`str r5, [r2, %10011]!`
Store Register (Register) | `str{cond}{b} <Rt>, [<Rn>, +/-<Rm>{, <shift>}]{!}`<br>`str{cond}{b} <Rt>, [<Rn>], +/-<Rm>{, <shift>}` | `str r6, [r2, r3]`<br>`streq r7, [r1, -r2]!`<br>`strb r4, [r3, r4, ror #2]`<br>`streqb r2, [r1], r5, lsl #1`
Store Register Unprivileged (Immediate) | `str{cond}{b}t <Rt>, [<Rn>]{, <imm12>}` | `strbt r2, [r6]`<br>`streqt r7, [r1], #6`
Store Register Unprivileged (Register) | `str{cond}{b}t <Rt>, [<Rn>], +/-<Rm>{, <shift>}` | `strt r4, [r3], r5`<br>`strbt r1, [r6], -r2, lsr #2`
Store Halfword / Signed Halfword / Signed Byte (Immediate) | `str{cond}<h/sh/sb> <Rt>, [<Rn>{, #+/-<imm8>}]{!}`<br>`str{cond}<h/sh/sb> <Rt>, [<Rn>], #+/-<imm8>` | `strsh r1, [r2]`<br>`strh r3, [r4,#-6]`<br>`strneh r2, [r3], $A`<br>`strsb r5, [r2, %10011]!`
Store Halfword / Signed Halfword / Signed Byte (Literal) | `str{<cond>}<h/sh/sb> <Rt>, <label>` | `strh r2, num1`
Store Halfword / Signed Halfword / Signed Byte (Register) | `str{cond}<h/sh/sb> <Rt>, [<Rn>, +/-<Rm>]{!}`<br>`str{cond}<h/sh/sb> <Rt>, [<Rn>], +/-<Rm>` | `strh r6, [r2, r3]`<br>`streqh r7, [r1, -r2]!`<br>`strsh r0, [r4], r2`
Signed Multiply Accumulate Long | `umlal{<cond>}{s} <RdLo>, <RdHi>, <Rn>, <Rm>` | `umlal r0, r1, r5, r8`
Signed Multiply Long | `umull{<cond>}{s} <RdLo>, <RdHi>, <Rn>, <Rm>` | `umull r0, r1, r5, r8`
Subtract | `sub{<cond>}{s} <Rd>, <Rn>, <oprnd2> {, <shift>}` | `sub r0, r0, r1`<br>`sub r3, r12, #127`<br>`sub r2, r3, r4, asr r5`
Software Interrupt | `swi{<cond>} <imm24>` | `swi $250000`
Swap | `swp{<cond>}{b} <Rt>, <Rt2>, [<Rn>]` | `swp r4, r2, [r7]`<br>`swpb r1, r2, [r4]`<br>`swpneb r9, r6, [r7]`
Test Equivalence | `teq{<cond>}{s} <Rn>, <oprnd2> {, <shift>}` | `teq r0, r2`<br>`teq r8,$6000000`<br>`teqeq r5, r3, lsr #3`
Test | `tst{<cond>}{s} <Rn>, <oprnd2> {, <shift>}` | `tst r0, r2`<br>`tst r8,$6000000`<br>`tsteq r5, r3, lsr #3`
Unsigned Multiply Accumulate Long | `umlal{<cond>}{s} <RdLo>, <RdHi>, <Rn>, <Rm>` | `umlal r0, r1, r5, r8`
Unsigned Multiply Long | `umull{<cond>}{s} <RdLo>, <RdHi>, <Rn>, <Rm>` | `umull r0, r1, r5, r8`

### Pseudo-Instructions

Name | Instruction | Example Usage
---- | ----------- | -------------
Load Address (Small-Range) | `adr{<cond>} <Rd>, <label>` | `adr r4, sprite`
Arithmetic Shift Right | `asr{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `asr r0, r0, #4`<br>`asrne r2,r4,r3`
Logical Shift Left | `lsl{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `lsl r0, r0, #4`<br>`lslne r2,r4,r3`
Logical Shift Right | `lsr{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `lsr r0, r0, #4`<br>`lsrne r2,r4,r3`
Negate Register | `neg{<cond>} <Rd>, <Rm>` | `neg r2, r4`<br>`negeq r1, r1`
No Operation | `nop` | `nop`
Pop Multiple Registers | `pop{<cond>} <register_list>` | `pop {r0-r2, r4, r14}`
Push Multiple Registers | `push{<cond>} <register_list>` | `push {r0-r2, r4, r14}`
Rotate Right | `ror{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `ror r0, r0, #4`<br>`rorne r2,r4,r3`
Rotate Right with Extend | `rrx{<cond>}{s} <Rd>, <Rm>` | `rrx r2, r1`
Supervisor Call | `svc{<cond>} <imm24>` | `svc $250000`
Generate Undefined Instruction | `und{<cond>} {#expr}` | `und`<br>`und #1234`
