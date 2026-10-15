# off-by-one — Writeup

| | |
|---|---|
| **Source** | [crackmes.one](https://crackmes.one/crackme/68692100aadb6eeafb398fd3) |
| **Type** | Crackme, Linux x86-64 ELF (raw syscalls, no libc) |
| **Tools** | GDB |
| **Goal** | Find the accepted passphrase |
| **Passphrase** | `DXUPWYfU` |

## 1. Overview

The program prompts for a passphrase and reports whether it is correct. The analysis was done dynamically in GDB, stepping through the disassembly and inspecting registers and memory at each step.

## 2. I/O through raw syscalls

The binary performs its I/O with `syscall` instructions rather than library calls.

**Prompt (`sys_write`)** — at the first `syscall`, `rax = 1` selects `sys_write`, with `rdi = 1` (stdout), `rsi` pointing to the message and `rdx = 0xc` bytes:

```
=> 0x401150:  syscall
   ...
$rax = 0x1   $rdi = 0x1   $rdx = 0xc   $rsi = 0x401199
(gdb) x/s $rsi
0x401199: "Passphrase: ..."
```

**Input (`sys_read`)** — the next `syscall` has `rax = 0` (`sys_read`), `rdi = 0` (stdin), `rsi = 0x402000` (destination buffer) and `rdx = 0x1e` (up to 30 bytes):

```
=> 0x40116b:  syscall
$rax = 0x0   $rdi = 0x0   $rdx = 0x1e   $rsi = 0x402000
```

After entering `AAA`, the buffer contains the input including the newline:

```
(gdb) ni
AAA
(gdb) x/s $rsi
0x402000: "AAA\n"
```

## 3. Comparison loop

After the read, the program loads the address of the failure message into `r11`:

```
=> 0x4010ae:  xor    %r10,%r10
   0x4010b1:  movabs $0x401069,%r11
   0x4010bb:  cmp    $0x8,%rcx
   0x4010bf:  je     0x401129
   ...
(gdb) x/s 0x401069
0x401069: "The given password is unfortunately incorrect\n"
```

The loop body, as seen in GDB, is:

```
0x4010bb:  cmp    $0x8,%rcx
0x4010bf:  je     0x401129
0x4010c1:  inc    %rdx
0x4010c4:  mov    (%r11),%r10b
0x4010c7:  xor    %rax,%rax
0x4010ca:  xor    %rdx,%rdx
0x4010cd:  mov    %r10,%rax
0x4010d0:  div    %rbx
0x4010d3:  add    $0x30,%dl
0x4010d6:  cmp    %dl,0x402000(%rcx)
0x4010dc:  jne    0x401130
0x4010de:  inc    %rcx
0x4010e1:  inc    %r11
```

My first hypothesis was that `cmp $0x8,%rcx` checked the length of the input, so I re-ran the program with `1 2 3 4 5 6 7 8`. `rcx` did not change, so that hypothesis was wrong.

Stepping further showed that the program only takes the failure path after the `cmp` at `0x4010d6`. At that point `dl` holds the expected character for the current position, and the input byte at `0x402000 + rcx` is compared against it.

## 4. Solution

On the first iteration `rcx = 0` and `dl = 0x44`, which is `D`:

```
=> 0x4010d6:  cmp    %dl,0x402000(%rcx)
$rcx = 0x0   $rdx = 0x44
(gdb) x/s 0x402000
0x402000: "D\n"
(gdb) x/x 0x402000
0x402000: 0x44
```

Re-running the program with `D` as the input makes the comparison succeed and `rcx` is incremented, which indicates that the program checks the passphrase one character at a time and that `rcx` is the character index. The loop ends when `rcx` reaches 8, so the passphrase has 8 characters.

On the second iteration (`rcx = 1`), `dl = 0x58`, which is `X`:

```
=> 0x4010d6:  cmp    %dl,0x402000(%rcx)
$rcx = 0x1   $rdx = 0x58
```

The third iteration gives `rcx = 2`, `dl = 0x55`, which is `U`. Repeating the same procedure for each position yields:

| Position | `dl` | Char |
|----------|------|------|
| 0 | 0x44 | D |
| 1 | 0x58 | X |
| 2 | 0x55 | U |
| 3 | 0x50 | P |
| 4 | 0x57 | W |
| 5 | 0x59 | Y |
| 6 | 0x66 | f |
| 7 | 0x55 | U |

```
$ ./off-by-one
Passphrase: DXUPWYfU
Yes! You found the correct passphrase
```
