#ifndef __PRIMETESTER_C
#define __PRIMETESTER_C 1

#include <stdlib.h>
#include <time.h>
#include <stdio.h>


#include "primetester.h"

PTYPE mod_pow(PTYPE base, PTYPE exp, PTYPE mod)
{
    register PTYPE result = 1;
    while (exp) {
        if (exp & 1)
            result = result * base % mod;
        exp >>= 1;
        base = (base * base) % mod;
    }
    return result;
}

int is_prime(PTYPE num, int k)
{
    register int i;
    PTYPE a;
    for (i = 0; i < k; i++) {
        a = rand() % (num - 3) + 2;
        if (mod_pow(a, num - 1, num) != 1)
            return 0;
    }
    return 1;
}

PTYPE next_prime(PTYPE prime)
{
    srand(time(NULL));
    if (prime % 2 == 0)
        prime ++;
    register PTYPE orig = prime;

    while (!is_prime(orig, 3) && (orig - prime) < 5000) {
        orig += 2;
    }
    if (orig <= prime) {
        fprintf(stderr, "WTF?!?\n");
    }
    return orig;
}

#endif
