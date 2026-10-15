# Orbit Fold (jeffli6789) — Writeup

| | |
|---|---|
| **Source** | [crackmes.one](https://crackmes.one/crackme/6ab1e2bf5c861323b6300702) |
| **Type** | Crackme, Linux x86-64 ELF (PIE) |
| **Tools** | Ghidra, GDB, C, qalc |
| **Goal** | Recover the accepted flag |
| **Flag** | `CMO{orbit_folded_twice}` |
| **Code** | [`src/orbit_fold/solver.c`](../src/orbit_fold/solver.c) |

## 1. Overview

The program accepts a flag either as `argv[1]` or from a `Flag:` prompt and validates it in two independent stages.

```
Orbit Fold - recover the accepted flag.
Flag: CMO{orbit_folded_twice}
Correct. Submit that flag to claim your points.
```

## 2. Input handling and length check

```c
__s = local_buffer;                 // 136-byte array
puts("Orbit Fold - recover the accepted flag.");
if (param_1 == 2) {
    __s = *(char **)(param_2 + 8);  // argv[1]
}
sVar6 = strlen(__s);
if (sVar6 == 0x17) {
    lVar7 = 1;
    iVar9 = 0;
    ...
```

`param_2` is treated as a `long`, so adding 8 moves from the program name (`argv[0]`) to the first argument. The flag must be exactly **23 characters** (0x17).

## 3. Stage 1 — weighted checksum

```c
do {
    lVar3 = lVar7 + -1;
    iVar5 = (int)lVar7;
    lVar7 = lVar7 + 1;
    iVar9 = iVar9 + (uint)(byte)__s[lVar3] * iVar5;
} while (lVar7 != 0x18);

if ((short)iVar9 == 0x728a) { ... }
```

The loop runs once per character and computes the sum of `char[i] * (i + 1)`. The low 16 bits of the result must equal `0x728a`. I reimplemented the loop in C and tried strings made from letters of the alphabet until one satisfied the condition:

```
$ ./replica abaaaaahijklmnopqrstaaa
__s == 97

 len(__s) == 0X17 or 23
iVar9 == 0x61
iVar9 == 0x125
iVar9 == 0x248
...
iVar9 == 0x69d3
iVar9 == 0x728a

short(iVar9) == 0x728a
```

## 4. Stage 2 — per-character transformation

```c
pbVar10 = &DAT_001020a0;
bVar11 = 0;
do {
    bVar1 = *pbVar10;
    pbVar10 = pbVar10 + 1;
    bVar4 = bVar1 * '\a' + 0x31 ^ __s[bVar1];
    bVar2 = bVar1 % 5 + 1 & 7;
    bVar11 = bVar11 | (bVar4 << bVar2 | bVar4 >> 8 - bVar2) + (bVar1 * '\r' ^ 0x5a) ^
                      (&DAT_00102080)[bVar1];
} while (pbVar10 != (byte *)0x1020b7);
```

The two tables referenced by the loop were located in the disassembly and dumped in GDB:

```
55555555511b  LEA  RSI,[0x5555555560a0]        ; DAT_001020a0
555555555124  LEA  R9,[0x555555556080]         ; DAT_00102080

(gdb) x/23x 0x5555555560a0
0x5555555560a0: 0x07  0x00  0x13  0x03  0x0e  0x16  0x05  0x0b
0x5555555560a8: 0x01  0x11  0x09  0x14  0x06  0x0d  0x02  0x10
0x5555555560b0: 0x08  0x15  0x04  0x0c  0x12  0x0a  0x0f

(gdb) x/25x $r9
0x555555556080: 0x3e  0x2c  0xc3  0x50  0xb2  0x67  0xf8  0x59
0x555555556088: 0x03  0x14  0xfa  0x19  0x15  0x81  0xca  0x96
0x555555556090: 0x85  0x6d  0x3d  0xa8  0x1b  0xd1  0xf9  0x00
0x555555556098: 0x00
```

- **Index table** (`DAT_001020a0`, 23 bytes): selects which flag character is tested in each iteration (`bVar1`).
- **Value table** (`DAT_00102080`): `table[bVar1]` is the constant the transformed character is compared against.

The result of every iteration is OR-ed into `bVar11`, which must end up as **0**. Therefore every iteration must contribute 0, and each character can be solved independently of the others.

A C replica of this loop, with a trace per iteration, was used to check the logic (shown here for the first iteration, with the test string from stage 1):

```
Count 0
    bVar1 == list[0] -> 0x7
    bVar4 = 0x7 * 7 + 0x31 ^ __s[0x7] (0x68, 'h')  -> bVar4 == 0xa
    bVar2 == 0x3
    temp1 = 0xa << 0x3 | 0xa >> 8 - 0x3            -> temp1 == 0x50
    temp2 = 0x7 * 13 ^ 0x5a                        -> temp2 == 0x1
    bVar11 = 0 | 0x50 + 0x1 ^ 0x59                 -> bVar11 == 0x8
```

The replica also confirms that stage 2 is independent of stage 1 and runs exactly 23 iterations (`count == 23`).

## 5. Solving one character by hand

For an iteration to contribute 0, the final expression must be 0, i.e. `(temp1 + temp2) ^ table[bVar1] == 0`, where `temp1` is the rotated `bVar4` and `temp2 = bVar1 * 13 ^ 0x5a`.

**Iteration 1** (`bVar1 = 7`, position 7): `temp2 = 0x01` and `table[7] = 0x59`, so `temp1` must be `0x58`.

```
$ qalc -b 16 "0 | 0x58 + 0x1 xor 0x59"
0 | ((88 + 1) xor 89) = 0x0

$ qalc -b 16 "0xb << 0x3 | 0xb >> 8 - 0x3"       ; which bVar4 gives temp1 = 0x58?
shift(11, 3) | shift(11, −(8 − 3)) = 0x58

$ qalc -b 16 "0x7 * 0x7 + 0x31 xor 0x69"         ; which character gives bVar4 = 0xb?
((7 × 7) + 49) xor 105 = 0xB
```

So position 7 must hold `0x69` (`i`).

**Iteration 2** (`bVar1 = 0`, position 0): `temp2 = 0x5a` and `table[0] = 0x3e`, so `temp1 = 0x3e - 0x5a = -0x1c`, which is `0xe4` as an unsigned byte. With `bVar2 = 1`, the rotation must satisfy:

```
(bVar4 << 1) | (bVar4 >> 7) = 0xe4 = 11100100
(abcdefgh << 1) | (abcdefgh >> 7) = bcdefgha = 11100100
→ bVar4 = 01110010 = 0x72

bVar4 = (0 * 7 + 0x31) ^ char = 0x72   →   char = 0x31 ^ 0x72 = 0x43   ('C')
```

The same procedure gives position `0x13` = `i`, position `0x03` = `{`, and position `0x0e` = `e`.

## 6. Automating the solution

Because the characters are independent, the C replica of stage 2 was turned into a solver. For each entry of the index table it tries every byte value until the iteration contributes 0:

```c
static const unsigned char index_table[23] = {
    0x07,0x00,0x13,0x03,0x0e,0x16,0x05,0x0b,0x01,0x11,0x09,0x14,
    0x06,0x0d,0x02,0x10,0x08,0x15,0x04,0x0c,0x12,0x0a,0x0f };
static const unsigned char value_table[23] = {
    0x3e,0x2c,0xc3,0x50,0xb2,0x67,0xf8,0x59,0x03,0x14,0xfa,0x19,
    0x15,0x81,0xca,0x96,0x85,0x6d,0x3d,0xa8,0x1b,0xd1,0xf9 };

int main(void)
{
    char flag[24] = {0};

    for (int i = 0; i < 23; i++) {
        unsigned char idx = index_table[i];
        for (int c = 0; c < 256; c++) {
            unsigned char b4  = (unsigned char)(idx * 7 + 0x31) ^ (unsigned char)c;
            unsigned char b2  = (idx % 5 + 1) & 7;
            unsigned char rot = (unsigned char)(b4 << b2 | b4 >> (8 - b2));
            unsigned char t2  = (unsigned char)(idx * 13) ^ 0x5a;
            if ((unsigned char)(rot + t2) == value_table[idx]) { flag[idx] = (char)c; break; }
        }
    }
    puts(flag);
}
```

Output:

```
CMO{orbit_folded_twice}
```

## 7. Result

```
$ ./orbit_fold_linux_x86_64
Orbit Fold - recover the accepted flag.
Flag: CMO{orbit_folded_twice}
Correct. Submit that flag to claim your points.
```
