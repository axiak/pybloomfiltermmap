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

/* Create a bloom filter without a memory-mapped file backing it */
BloomFilter *bloomfilter_Create_Malloc(size_t max_num_elem, double error_rate,
                                BTYPE num_bits, int *hash_seeds, int num_hashes);

/* Create a bloom filter with a memory-mapped file backing it */
BloomFilter *bloomfilter_Create_Mmap(size_t max_num_elem, double error_rate,
                                const char * file, BTYPE num_bits, int oflags, int perms,
                                int *hash_seeds, int num_hashes);

void bloomfilter_Destroy(BloomFilter * bf);

int bloomfilter_Update(BloomFilter * bf, char * data, int size);

BloomFilter * bloomfilter_Copy_Template(BloomFilter * src, char * filename, int perms);

/* A lot of this is inlined.. */
BTYPE _hash_char(uint32_t hash_seed, Key * key);

BTYPE _hash_long(uint32_t hash_seed, Key * key);


static inline int bloomfilter_Add(BloomFilter * bf, Key * key)
{
    BTYPE (*hashfunc)(uint32_t, Key *) = _hash_char;
    register BTYPE mod = bf->array->bits;
    register int i;
    register int result = 1;
    register BTYPE hash_res;

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
    register BTYPE (*hashfunc)(uint32_t, Key *) = _hash_char;
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


static inline int bloomfilter_Test_Add(BloomFilter * bf, Key * key)
{
    BTYPE (*hashfunc) (uint32_t, Key *) = key->shash == NULL ? _hash_long : _hash_char;
    BTYPE mod = bf->array->bits;
    int i;
    int result = 1;
    BTYPE hash_res;

    for (i = bf->num_hashes - 1; i >= 0; --i) {
        hash_res = (*hashfunc)(bf->hash_seeds[i], key) % mod;
        result &= mbarray_Test_Set(bf->array, hash_res);
    }
    if (!result && bf->count_correct) {
        bf->elem_count ++;
    }
    return result;
}
__attribute__((always_inline))

#endif
