#ifndef __PRIMETESTER_C
#define __PRIMETESTER_C 1

#include <stdlib.h>
#include <time.h>
#include <stdio.h>


#include "primetester.h"

PTYPE next_prime(PTYPE prime)
{
    register PTYPE initial_prime = 89;
    while (initial_prime < prime) {
        initial_prime <<= 1;
        ++initial_prime;
    }
    return initial_prime;
}

#endif
