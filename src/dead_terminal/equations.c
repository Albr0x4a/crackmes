/*
 * Dead Terminal (soulreaper_shell) - equation extractor
 *
 * Prints the equation checked for each character of the key, using the
 * constants taken from the decompiled validation function (FUN_00101560).
 * Each line has the form:  expected = (X ^ 0x2a) + k
 *
 * Build:  gcc -Wall -o equations equations.c
 * Usage:  ./equations
 */
#include <stdio.h>

int main(void)
{
    char expected[8] = { '\x7f', 'y', 'x', -0x76, -0x7e, -0x72, '7', '4' };
    char *p = expected;
    char k = '\a';

    do {
        printf("%#x = (X ^ 0x2a) + %#x\n", *p, k);
        k = k + '\x03';
        p = p + 1;
    } while (k != '\x1f');

    return 0;
}
