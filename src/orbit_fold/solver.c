/*
 * Orbit Fold - flag solver
 *
 * Stage 2 of the validation tests each flag character independently, using
 * two 23-byte tables dumped from the binary. For every table entry, this
 * program tries all byte values until the check contributes 0.
 *
 * Build:  gcc -Wall -o solver solver.c
 * Usage:  ./solver
 */
#include <stdio.h>

/* DAT_001020a0: which flag position is tested in each iteration */
static const unsigned char index_table[23] = {
    0x07, 0x00, 0x13, 0x03, 0x0e, 0x16, 0x05, 0x0b, 0x01, 0x11, 0x09, 0x14,
    0x06, 0x0d, 0x02, 0x10, 0x08, 0x15, 0x04, 0x0c, 0x12, 0x0a, 0x0f };

/* DAT_00102080: constant compared against, indexed by flag position */
static const unsigned char value_table[23] = {
    0x3e, 0x2c, 0xc3, 0x50, 0xb2, 0x67, 0xf8, 0x59, 0x03, 0x14, 0xfa, 0x19,
    0x15, 0x81, 0xca, 0x96, 0x85, 0x6d, 0x3d, 0xa8, 0x1b, 0xd1, 0xf9 };

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
            if ((unsigned char)(rot + t2) == value_table[idx]) {
                flag[idx] = (char)c;
                break;
            }
        }
    }

    puts(flag);
    return 0;
}
