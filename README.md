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

### Pseudo-Instructions

## Supported Assembler Directives

Directive | Description | Example Usage
--------- | ----------- | -------------
`@b <byte list>` | Define bytes | `@b $10, #255`
`@h <halfword list>` | Define halfwords | `@h %10000000011`
`@w <word list>` | Define words | `@w $05000200, $4000130, %110000000000000000000000010`

