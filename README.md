# GBA ARM Assembler

I don't know what I'm doing so I'm learning.

---

## Build Instructions

Requires pthread.

```sh
gcc -fshort-enums main.c -pthread -o gbaaa
```

---

## Supported Instructions

- `mov{cond}{s} Rd, <Oprnd2>`

### Pseudo-Instructions

