/*
 * Hard Crackme (Sigil Forging Ritual) - keygen
 *
 * Reimplements the name-hashing stage of the validation function and prints
 * the 24-hex-character sigil that matches a given name.
 *
 * Build:  gcc -Wall -o keygen keygen.c
 * Usage:  ./keygen <name>          (name: 5-32 characters)
 * Example: ./keygen asdfg   ->   799c1837cc2b3372d8eba7e6
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <name>\n", argv[0]);
        return 1;
    }

    const unsigned char *name = (const unsigned char *)argv[1];
    size_t len = strlen(argv[1]);
    if (len < 5 || len > 32) {
        fprintf(stderr, "name must be 5-32 characters\n");
        return 1;
    }

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
