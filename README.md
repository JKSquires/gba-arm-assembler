# GBA ARM Assembler

I don't know what I'm doing so I'm learning.

I just know that I'm going to look back at this code in the future and think, "wow was I bad at programming," and that's a good thing.

---

## Build Instructions

Requires pthread.

```sh
gcc -fshort-enums main.c -pthread -o gbaaa -s
```

---

## Supported Instructions

Name | Instruction | Example Usage
---- | ----------- | -------------
Branch | `b{<cond>} <label>` | `b start`
Branch with Link | `bl{<cond>} <label>` | `bl div`
Branch and Exchange | `bx{<cond>} <Rm>` | `bx r14`

### Pseudo-Instructions

Name | Instruction | Example Usage
---- | ----------- | -------------
No Operation | `nop` | `nop`

## Supported Assembler Directives

Name | Directive | Example Usage
---- | --------- | -------------
Define bytes | `@b <byte list>` | `@b $10, #255`
Define halfwords | `@h <halfword list>` | `@h %10000000011`
Define words | `@w <word list>` | `@w $05000200, $4000130, %110000000000000000000000010`

