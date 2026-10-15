# Hard Crackme (Sigil Forging Ritual) — Writeup

| | |
|---|---|
| **Type** | Crackme, Linux x86-64 ELF |
| **Tools** | Ghidra, GDB, C, qalc |
| **Goal** | Produce a valid 24-hex-character sigil for a given name |
| **Example** | name `asdfg` → sigil `799c1837cc2b3372d8eba7e6` |

## 1. Overview

The program asks for a name (5–32 characters) and a 24-character hexadecimal sigil. Initial runs show the length rules and the failure message:

```
$ ./hard_crackme
=== sigil forging ritual v2.0 ===
weave a name (5-32 chars) and bind it with a 24-hex sigil.
name: testing
sigil: 1234
the sigil must be exactly 24 hex characters.

$ ./hard_crackme
name: testing
sigil: 123456789012345678901234
the spirits reject you.
```

The sigil is not a fixed value: it is derived from the name. The task is therefore to reproduce the derivation.

## 2. Anti-debugging check

At the start of the function the binary calls `ptrace(PTRACE_TRACEME, 0, 1, 0)`:

```c
lVar26 = ptrace(PTRACE_TRACEME, 0, 1, 0);
if (lVar26 == -1) {
    puts("the spirits sense a watcher...");
}
```

The call fails (returns -1) when a debugger is already attached, and the program then prints the message above and exits.

To bypass it in GDB: set a breakpoint before the `ptrace` call, step over it, and change the value of `RAX` so that the comparison with -1 no longer detects the debugger.

## 3. Input validation

Ghidra shows a single function (`FUN_001010c0`) that performs the whole check. The first stage validates the name:

```c
pcVar27 = fgets(local_98, 0x40, stdin);
sVar28 = strcspn(local_98, "\n");
local_98[sVar28] = 0;
sVar28 = strlen(local_98);
if (sVar28 - 5 < 0x1c) {
    pcVar27 = local_98;
    do {
        if (0x5e < (byte)(*pcVar27 - 0x20U)) {
            puts("strange characters detected.");
            return 1;
        }
        pcVar27 = pcVar27 + 1;
    } while (pcVar27 != local_98 + sVar28);
```

- The newline is removed and the length is checked: `strlen(name) - 5 < 0x1c`, i.e. **5 to 32 characters**.
- Every character must be printable ASCII (`0x20`–`0x7e`); otherwise the program prints `strange characters detected.`

The second stage reads the sigil and requires a length of exactly **0x18 (24) characters**:

```c
pcVar27 = fgets(local_58, 0x40, stdin);
sVar28 = strcspn(local_58, "\n");
local_58[sVar28] = 0;
sVar28 = strlen(local_58);
if (sVar28 == 0x18) { ... }
```

## 4. Name hashing

Three 32-bit state variables are initialised and updated once per character of the name (variable names follow the Ghidra listing):

```c
lVar26 = 1;
uVar29 = 0x811c9dc5;
uVar25 = 0x9e3779b9;
uVar28 = 0xcbf29ce4;
bVar32 = 0;
do {
    bVar2 = *pointer_user_input3;
    pointer_user_input3 = pointer_user_input3 + 1;
    iVar31 = (int)lVar26;
    lVar26 = lVar26 + 1;
    uVar29 = (uVar29 ^ bVar2) * 0x1000193;
    uVar28 = (uint)bVar2 * iVar31 + uVar28;
    uVar28 = uVar28 * 0x80 | uVar28 >> 0x19;
    uVar25 = (uint)bVar2 << (bVar32 & 0xf) ^ uVar25;
    uVar25 = uVar25 << 0xd | uVar25 >> 0x13;
    bVar32 = bVar32 + 3;
} while (sVar27 + 1 != lVar26);
```

After the loop, the three values go through several mixing rounds (XOR with constants such as `0xdeadbeef` and `0xcafebabe`, multiplications by `0x7feb352d` and `-0x7b935975`, and xor-shifts):

```c
uVar29 = ((uVar29 ^ 0xdeadbeef) >> 0x10 ^ uVar29 ^ 0xdeadbeef) * 0x7feb352d;
uVar29 = (uVar29 ^ uVar29 >> 0xf) * -0x7b935975;
uVar29 = uVar29 ^ uVar29 >> 0x10;
uVar28 = uVar28 ^ uVar29;
uVar28 = (uVar28 >> 0x10 ^ uVar28) * 0x7feb352d;
uVar28 = (uVar28 ^ uVar28 >> 0xf) * -0x7b935975;
uVar28 = uVar28 ^ uVar28 >> 0x10;
uVar25 = uVar25 ^ uVar28 ^ 0xcafebabe;
uVar25 = (uVar25 >> 0x10 ^ uVar25) * 0x7feb352d;
uVar25 = (uVar25 ^ uVar25 >> 0xf) * -0x7b935975;
uVar29 = uVar29 ^ uVar25 ^ uVar25 >> 0x10;
snprintf((char *)&local_b8, 0x19, "%08x%08x%08x", (ulong)uVar29, (ulong)(uVar28 ^ uVar29));
```

The results are formatted into a 24-character hex string with `snprintf`. The function then compares that string with the sigil supplied by the user and prints `the pact is sealed` when they match.

## 5. Recovering the third `snprintf` argument

Ghidra's decompilation passes only two values to a format string with three `%08x` conversions. To find the third one, I inspected the call in GDB. The register state at the `snprintf` call shows the 5th and 6th arguments in `r8` and `r9`:

```
=> 0x55555555535c:  call   0x555555555060 <snprintf@plt>
$rdi = 0x7fffffffdbe0   $rsi = 0x19   $rdx = 0x55555555614a   $rcx = 0x799c1837
$r8  = 0xcc2b3372       $r9  = 0xd8eba7e6
```

The instructions just before the call show how `r8` and `r9` are produced:

```
=> 0x55555555534b:  mov    %r9d,%eax
   0x55555555534e:  shr    $0x10,%eax
   0x555555555351:  xor    %eax,%r9d
   0x555555555354:  xor    %eax,%eax
   0x555555555356:  xor    %r9d,%ecx
   0x555555555359:  xor    %ecx,%r8d
   0x55555555535c:  call   0x555555555060 <snprintf@plt>
```

The `shr $0x10` followed by `xor` on `r9d` is the `(x >> 16) ^ x` pattern. To confirm the values, I reimplemented the algorithm in C (`./replica`), printed the intermediate variables for the name `asdfg`, and checked the operations with `qalc`:

```
$ ./replica asdfg 123456789012345678901234 | grep -iE "uvar28|uvar29"
uVar29 = 0x799c1837
uVar28 = 0xb5b72b45
$ qalc -b 16 "0xb5b72b45 xor 0x799c1837"
3048680261 xor 2040272951 = 0xCC2B3372          ← matches r8

$ ./replica asdfg 123456789012345678901234 | grep -i "uvar25"
uVar25 = 0xd8eb7f0d
$ qalc -b 16 "(0xd8eb7f0d >> 0x10) xor 0xd8eb7f0d"
shift(3639312141, −16) xor 3639312141 = 0xD8EBA7E6   ← matches r9
```

The complete call is therefore:

```c
snprintf(buf, 0x19, "%08x%08x%08x",
         uVar29, uVar28 ^ uVar29, (uVar25 >> 0x10) ^ uVar25);
```

The buffer passed as the first argument confirms the result:

```
(gdb) x/s 0x7fffffffdbe0
0x7fffffffdbe0: "799c1837cc2b3372d8eba7e6"
```

## 6. Verification

Entering the name `asdfg` with the generated string is accepted:

```
name: asdfg
sigil: 799c1837cc2b3372d8eba7e6
the pact is sealed. well done, sigil-smith.
```

Because the sigil depends only on the name, the C reimplementation produces a valid sigil for any name that passes the validation rules.

## 7. Keygen

The name-hashing stage and the final output format, reduced to the code that matters:

```c
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
    if (argc != 2) { fprintf(stderr, "usage: %s <name>\n", argv[0]); return 1; }
    const unsigned char *name = (const unsigned char *)argv[1];
    size_t len = strlen(argv[1]);
    if (len < 5 || len > 32) { fprintf(stderr, "name must be 5-32 characters\n"); return 1; }

    uint32_t a = 0x9e3779b9, b = 0xcbf29ce4, c = 0x811c9dc5;   /* uVar25, uVar28, uVar29 */
    unsigned char shift = 0;

    for (size_t i = 0; i < len; i++) {
        uint32_t ch = name[i];
        c = (c ^ ch) * 0x1000193;
        b = ch * (uint32_t)(i + 1) + b;
        b = b << 7 | b >> 25;
        a = (ch << (shift & 0xf)) ^ a;
        a = a << 13 | a >> 19;
        shift += 3;
    }

    c = ((c ^ 0xdeadbeef) >> 16 ^ c ^ 0xdeadbeef) * 0x7feb352d;
    c = (c ^ c >> 15) * (uint32_t)-0x7b935975;
    c = c ^ c >> 16;
    b = b ^ c;
    b = (b >> 16 ^ b) * 0x7feb352d;
    b = (b ^ b >> 15) * (uint32_t)-0x7b935975;
    b = b ^ b >> 16;
    a = a ^ b ^ 0xcafebabe;
    a = (a >> 16 ^ a) * 0x7feb352d;
    a = (a ^ a >> 15) * (uint32_t)-0x7b935975;
    c = c ^ a ^ a >> 16;

    printf("%08x%08x%08x\n", c, b ^ c, (a >> 16) ^ a);
    return 0;
}
```

```
$ ./keygen asdfg
799c1837cc2b3372d8eba7e6
```
