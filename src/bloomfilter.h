#ifndef __BLOOMFILTER_H
#define __BLOOMFILTER_H 1

#include <stdlib.h>
#include "mmapbitarray.h"
#define BF_CURRENT_VERSION 1

struct _BloomFilter {
    uint64_t max_num_elem;
    double error_rate;
    uint32_t num_hashes;
    uint32_t hash_seeds[256];
    /* All of the bit data is already in here. */
    MBArray * array;
    unsigned char bf_version;
    unsigned char count_correct;
    uint64_t elem_count;
    uint32_t reserved[32];
};

typedef struct {
    uint64_t nhash;
    char * shash;
} Key;

typedef struct _BloomFilter BloomFilter;

BloomFilter *bloomfilter_Create(size_t max_num_elem, double error_rate,
                                const char * file, BTYPE num_bits, int oflags, int perms,
                                int *hash_seeds, int num_hashes);

void bloomfilter_Destroy(BloomFilter * bf);

MBArray * mbarray_And_Ternary(MBArray * dest, MBArray * a, MBArray * b);

MBArray * mbarray_Or_Ternary(MBArray * dest, MBArray * a, MBArray * b);

MBArray * mbarray_Xor_Ternary(MBArray * dest, MBArray * a, MBArray * b);

MBArray * mbarray_Copy_Template(MBArray * src, char * filename, int perms);

int mbarray_Update(MBArray * array, char * data, int size);
/*MBArray * mbarray_Copy(MBarray * src, const char * filename);*/

int mbarray_FileSize(MBArray * array);

char * mbarray_CharData(MBArray * array);

int bloomfilter_Update(BloomFilter * bf, char * data, int size);

BloomFilter * bloomfilter_Copy_Template(BloomFilter * src, char * filename, int perms);

/* A lot of this is inlined.. */
uint32_t _hash_char(uint32_t hash_seed, Key * key);

uint32_t _hash_long(uint32_t hash_seed, Key * key);


static inline int bloomfilter_Add(BloomFilter * bf, Key * key)
{
    uint32_t (*hashfunc)(uint32_t, Key *) = _hash_char;
    register BTYPE mod = bf->array->bits;
    register int i;
    register int result = 1;
    register uint32_t hash_res;

    if (key->shash == NULL)
        hashfunc = _hash_long;

    for (i = bf->num_hashes - 1; i >= 0; --i) {
        hash_res = (*hashfunc)(bf->hash_seeds[i], key) % mod;
        if (result && !mbarray_Test(bf->array, hash_res)) {
            result = 0;
        }
        if (mbarray_Set(bf->array, hash_res)) {
            return 2;
        }
    }
    if (!result && bf->count_correct) {
        bf->elem_count ++;
    }
    return result;
}
__attribute__((always_inline))


static inline int bloomfilter_Test(BloomFilter * bf, Key * key)
{
    register BTYPE mod = bf->array->bits;
    register uint32_t (*hashfunc)(uint32_t, Key *) = _hash_char;
    register int i;

    if (key->shash == NULL)
        hashfunc = _hash_long;

    for (i = bf->num_hashes - 1; i >= 0; --i) {
        if (!mbarray_Test(bf->array, (*hashfunc)(bf->hash_seeds[i], key) % mod)) {
            return 0;
        }
    }
    return 1;
}
__attribute__((always_inline))




#endif
