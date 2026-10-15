# Dead Terminal (`soulreaper_shell`) — Writeup

| | |
|---|---|
| **Source** | [crackmes.one](https://crackmes.one/crackme/6a77c5d1df981859694944b8) |
| **Type** | Crackme, Linux x86-64 ELF |
| **Tools** | Ghidra, GDB, C |
| **Goal** | Find the key accepted by the `reap <key>` command |
| **Key** | `REAPER42` |

## 1. Overview

The binary is a minimal interactive shell. Any command other than `exit` or `reap` is executed through `fork`/`execvp`; `reap <key>` validates a key and prints the access result.

```
$ ./soulreaper_shell
soulreaper shell >sdaf
execvp error: No such file or directory
soulreaper shell >asdf
execvp error: No such file or directory
```

Ghidra shows that the whole program logic lives in one function, which I renamed `Main_function`.

## 2. Anti-debugging check

Before entering the main loop, the function calls `Debugger_Detector()`:

```c
local_30 = *(long *)(in_FS_OFFSET + 0x28);
iVar2 = Debugger_Detector();
if (iVar2 == 0) {
    /* main loop */
} else {
    __printf_chk(2, "Debugger Detected");
}
```

The detector opens `/proc/self/status`, reads it line by line, converts a value with `strtol` and returns whether that value is non-zero:

```c
__stream = fopen("/proc/self/status", "r");
...
iVar1 = __isoc23_strtol(local_11e, 0, 10);
fclose(__stream);
bVar3 = iVar1 != 0;
```

The check can be bypassed from GDB by setting a breakpoint at the end of the function and forcing the return value:

```
(gdb) break *<last instruction of Debugger_Detector>
(gdb) continue
(gdb) set $rax = 0
(gdb) continue
```

## 3. Command parsing

Inside an infinite loop, the program reads a line, strips the newline and splits it on spaces:

```c
__printf_chk(2, "soulreaper shell >");
fflush(stdout);
pcVar3 = fgets(local_438, 0x400, stdin);
sVar4 = strcspn(local_438, "\n");
local_438[sVar4] = '\0';
pcVar3 = strtok(local_438, " ");
do {
    apcStack_640[lVar5] = pcVar3;           // token stored in the argument array
    pcVar3 = strtok((char *)0x0, " ");
    lVar1 = lVar5 + 1;
} while (pcVar3 != (char *)0x0 && lVar5 != 0x3f);   // at most 63 tokens
```

The first token is then compared with `"exit"` and `"reap"`:

```c
iVar2 = strcmp(apcStack_640[1], "exit");
if (iVar2 == 0) break;
iVar2 = strcmp(apcStack_640[1], "reap");
```

Entering `123` and inspecting the `strcmp` arguments in GDB confirms this behaviour:

```
(gdb) x/s $rsi
0x555555556094: "exit"
(gdb) x/s $rdi
0x7fffffffd540: "123"
```

If the first token is `reap` with no argument, the program prints `Usage : reap <key>`. Otherwise it calls `FUN_00101560`, which decides whether access is granted. Any other command is passed to `execvp` in a child process.

## 4. Key validation

```c
undefined8 FUN_00101560(byte *param_1)
{
  char local_18[8];
  local_18[0] = '\x7f';  local_18[1] = 'y';   local_18[2] = 'x';  local_18[3] = -0x76;
  local_18[4] = -0x7e;   local_18[5] = -0x72; local_18[6] = '7';  local_18[7] = '4';

  sVar1 = strlen((char *)param_1);
  uVar2 = 0;
  if (sVar1 == 8) {
    cVar4 = '\a';
    do {
      if (*pcVar3 != (byte)((*param_1 ^ 0x2a) + cVar4)) {
        uVar2 = 0;
        goto LAB_001015e1;
      }
      cVar4 = cVar4 + '\x03';
      param_1 = param_1 + 1;
      pcVar3 = pcVar3 + 1;
    } while (cVar4 != '\x1f');
    uVar2 = 1;
  }
  ...
}
```

Running `reap 12345678` under GDB and stopping at the call confirms that `$rdi` points to the supplied key:

```
(gdb) x/s $rdi
0x7fffffffd545: "12345678"
```

The function requires an **8-character key** and checks each character with:

```
expected[i] == (key[i] ^ 0x2a) + k_i        where k_0 = 7 and k_i = k_(i-1) + 3
```

The loop runs eight times (`k` = 7, 10, 13, …, 28) and stops when `k` reaches 0x1f.

## 5. Solution

A small C program was used to extract the equation for each position from the constants in the decompiled function:

```c
char local_18[8] = { '\x7f', 'y', 'x', -0x76, -0x7e, -0x72, '7', '4' };
char *pcVar3 = local_18;
char cVar4 = '\a';
do {
    printf("%#x = (X ^ 0x2a) + %#x\n", *pcVar3, cVar4);
    cVar4 = cVar4 + '\x03';
    pcVar3 = pcVar3 + 1;
} while (cVar4 != '\x1f');
```

```
0x7f = (X ^ 0x2a) + 0x7
0x79 = (X ^ 0x2a) + 0xa
0x78 = (X ^ 0x2a) + 0xd
0xffffff8a = (X ^ 0x2a) + 0x10
0xffffff82 = (X ^ 0x2a) + 0x13
0xffffff8e = (X ^ 0x2a) + 0x16
0x37 = (X ^ 0x2a) + 0x19
0x34 = (X ^ 0x2a) + 0x1c
```

Bytes above `0x7f` are printed sign-extended (`0xffffff8a`), so only the low byte is relevant. Each equation can be inverted independently:

```
key[i] = ((expected[i] - k_i) & 0xff) ^ 0x2a

0x7f = (X ^ 0x2a) + 0x7   →   X = (0x7f - 0x7) ^ 0x2a = 0x78 ^ 0x2a = 0x52   ('R')
```

| i | expected | k | expected − k | ^ 0x2a | Char |
|---|----------|------|------|------|------|
| 0 | 0x7f | 0x07 | 0x78 | 0x52 | R |
| 1 | 0x79 | 0x0a | 0x6f | 0x45 | E |
| 2 | 0x78 | 0x0d | 0x6b | 0x41 | A |
| 3 | 0x8a | 0x10 | 0x7a | 0x50 | P |
| 4 | 0x82 | 0x13 | 0x6f | 0x45 | E |
| 5 | 0x8e | 0x16 | 0x78 | 0x52 | R |
| 6 | 0x37 | 0x19 | 0x1e | 0x34 | 4 |
| 7 | 0x34 | 0x1c | 0x18 | 0x32 | 2 |

The key is `REAPER42`:

```
soulreaper shell >reap REAPER42
Access granted
```
