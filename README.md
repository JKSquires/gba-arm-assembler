# GBA ARM Assembler

I don't know what I'm doing so I'm learning.

I just know that I'm going to look back at this code in the future and think, "wow was I bad at programming!"

---

## Build Instructions

Compile the main C file.

Recommended way:

```sh
gcc -O2 -s -fshort-enums main.c -o gbaaa
```

But if you really wanted to, you could just run:

```sh
gcc main.c
```

---

## Supported Instructions

Name | Instruction | Example Usage
---- | ----------- | -------------
Branch | `b{<cond>} <label>` | `b start`
Branch with Link | `bl{<cond>} <label>` | `bl div`
Branch and Exchange | `bx{<cond>} <Rm>` | `bx r14`
Move | `mov{<cond>}{s} <Rd>, <oprnd2>` | `mov r0, r2` / `mov r8,$6000000` / `moveqs r5, r3, lsr #3`

### Pseudo-Instructions

Name | Instruction | Example Usage
---- | ----------- | -------------
Arithmetic Shift Right | `asr{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `asr r0, r0, #4` / `asrne r2,r4,r3`
Logical Shift Left | `lsl{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `lsl r0, r0, #4` / `lslne r2,r4,r3`
Logical Shift Right | `lsr{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `lsr r0, r0, #4` / `lsrne r2,r4,r3`
No Operation | `nop` | `nop`
Rotate Right | `ror{<cond>}{s} <Rd>, <Rm>, <oprnd2>` | `ror r0, r0, #4` / `rorne r2,r4,r3`
Rotate Right with Extend | `rrx{<cond>}{s} <Rd>, <Rm>` | `rrx r2, r1`

## Supported Assembler Directives

Name | Directive | Example Usage
---- | --------- | -------------
Define bytes | `@b <byte list>` | `@b $10, #255`
Define halfwords | `@h <halfword list>` | `@h %10000000011`
Define words | `@w <word list>` | `@w $05000200, $4000130, %110000000000000000000000010`

