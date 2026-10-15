# Linux Crackme Writeups

Technical writeups of Linux x86-64 crackmes solved as part of my self-directed training in reverse engineering. Each writeup documents the analysis from first run to solution, with the Ghidra decompilation, GDB sessions and C code used along the way.

These are learning exercises, not professional research. They are shared to show my analysis process and how I approach unknown binaries.

> **Spoiler warning:** every writeup includes the final solution (key, flag or keygen).

## Writeups

| Crackme | Platform | Techniques | Source |
|---|---|---|---|
| [Dead Terminal](writeups/dead_terminal.md) | Linux x86-64 ELF | Anti-debugging bypass, command parsing, byte-wise key validation inverted with a C helper | [crackmes.one](https://crackmes.one/crackme/6a77c5d1df981859694944b8) |
| [Hard Crackme (Sigil Forging Ritual)](writeups/hard_crackme.md) | Linux x86-64 ELF | `ptrace` anti-debugging bypass, custom hash reimplemented in C, keygen | Not available |
| [off-by-one](writeups/off_by_one.md) | Linux x86-64 ELF (raw syscalls, no libc) | Pure dynamic analysis in GDB, syscall tracing, character-by-character comparison | [crackmes.one](https://crackmes.one/crackme/68692100aadb6eeafb398fd3) |
| [Orbit Fold](writeups/orbit_fold.md) | Linux x86-64 ELF (PIE) | Decompiler output analysis, lookup tables dumped in GDB, manual inversion of byte operations, C solver | [crackmes.one](https://crackmes.one/crackme/6ab1e2bf5c861323b6300702) |

## Approach

1. **Triage:** run the binary, observe its inputs, outputs and error messages.
2. **Static analysis:** locate the validation logic in Ghidra and read the decompiled code.
3. **Dynamic analysis:** confirm hypotheses in GDB by inspecting registers, memory and call arguments at key points.
4. **Reimplementation:** reproduce the validation logic in C to test it in isolation and derive the solution.
5. **Verification:** run the original binary with the recovered input.

## Tools

- Ghidra: static analysis and decompilation
- GDB: dynamic analysis
- GCC / C: reimplementation of validation logic, solvers and keygens
- qalc: checking bitwise and arithmetic operations
- Linux virtual machine

## Author

Alberto Pérez — [LinkedIn](http://www.linkedin.com/in/alberto-p%C3%A9rez-3104252b9)
