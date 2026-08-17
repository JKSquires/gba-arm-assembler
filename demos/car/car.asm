b start

@i "./demos/header.asm"

@i "./demos/car/sprite.asm"

vram_char_data_1:
@w $06010020

start:
mov r0,$4000000

mov r1,%1010001000000
strh r1,[r0]

adr r1,palette
str r1,[r0,$D4]

mov r1,$5000000
orr r1,r1,$200
str r1,[r0,$D8]

mov r1,%10000100000000000000000000000000
orr r1,r1,#3
str r1,[r0,$DC]

adr r1,car_sprite
str r1,[r0,$D4]

ldr r1,vram_char_data_1
str r1,[r0,$D8]

mov r1,%10000100000000000000000000000000
orr r1,r1,#64
str r1,[r0,$DC]

mov r1,$7000000

mov r2,%0100000000000000
orr r2,r2,%0000000010010000
strh r2,[r1]

mov r2,%1000000000000000
strh r2,[r1,#2]

mov r2,%0000000000000001
strh r2,[r1,#4]

mainLoop:
waitForVBlankEnd:
ldrh r2,[r0,$4]
tst r2,#1
bne waitForVBlankEnd
waitForVBlankStart:
ldrh r2,[r0,$4]
tst r2,#1
beq waitForVBlankStart

ldrh r2,[r1,#2]

and r3,r2,%11111111
cmp r3,%11010000
orreq r2,r2,%0001000000000000

tst r2,%11111111
mvneq r3,%0001000000000000
andeq r2,r2,r3

tst r2,%0001000000000000
addeq r2,r2,#1
subne r2,r2,#1

strh r2,[r1,#2]

b mainLoop
