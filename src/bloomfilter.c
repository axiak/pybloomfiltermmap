#ifndef __BLOOMFILTER_C
#define __BLOOMFILTER_C
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

#include "bloomfilter.h"

BloomFilter *bloomfilter_Create(size_t max_num_elem, double error_rate,
                                const char * file, BTYPE num_bits, int oflags, int perms,
                                int *hash_seeds, int num_hashes)
{
    BloomFilter * bf = (BloomFilter *)malloc(sizeof(BloomFilter));
    MBArray * array;

    if (!bf) {
        return NULL;
    }

    bf->max_num_elem = max_num_elem;
    bf->error_rate = error_rate;
    bf->num_hashes = num_hashes;
    bf->count_correct = 1;
    bf->bf_version = BF_CURRENT_VERSION;
    bf->elem_count = 0;
    memset(bf->reserved, sizeof(uint32_t) * 32, 0);

    memset(bf->hash_seeds, sizeof(uint32_t) * 256, 0);
    memcpy(bf->hash_seeds, hash_seeds, sizeof(uint32_t) * num_hashes);
    array = mbarray_Create(num_bits, file, (char *)bf, sizeof(BloomFilter), oflags, perms);

    if (!array) {
        bloomfilter_Destroy(bf);
        return NULL;
    }

    /* After we create the new array object, this array may already
       have all of the bloom filter data from the file in the
       header info. 
       By calling mbarray_Header, we copy that header data
       back into this BloomFilter object.
    */
    if (mbarray_Header((char *)bf, array, sizeof(BloomFilter)) == NULL) {
        bloomfilter_Destroy(bf);
        mbarray_Destroy(array);
        return NULL;
    }

    /* Since we just initialized from a file, we have to
       fix our pointers */
    bf->array = array;

    return bf;
}


void bloomfilter_Destroy(BloomFilter * bf)
{
    if (bf) {
        if (bf->array) {
            mbarray_Destroy(bf->array);
            bf->array = NULL;
        }
        free(bf);
    }
}


void bloomfilter_Print(BloomFilter * bf)
{
    printf("<BloomFilter num: %lu, error: %0.3f, num_hashes: %d>\n",
           (unsigned long)bf->max_num_elem, bf->error_rate, bf->num_hashes);
}

int bloomfilter_Update(BloomFilter * bf, char * data, int size)
{
    MBArray * array = bf->array;
    int retval = mbarray_Update(bf->array, data, size);
    if (retval) {
        return retval;
    }
    if (mbarray_Header((char *)bf, array, sizeof(BloomFilter)) == NULL) {
        return 1;
    }
    bf->array = array;
    return 0;
}


BloomFilter * bloomfilter_Copy_Template(BloomFilter * src, char * filename, int perms)
{
    BloomFilter * bf = (BloomFilter *)malloc(sizeof(BloomFilter));
    MBArray * array;

    if (bf == NULL) {
        return NULL;
    }

    array = mbarray_Copy_Template(src->array, filename, perms);
    if (array == NULL) {
        free(bf);
        return NULL;
    }

    if (mbarray_Header((char *)bf, array, sizeof(BloomFilter)) == NULL) {
        bloomfilter_Destroy(bf);
        mbarray_Destroy(array);
        return NULL;
    }

    bf->array = array;
    return bf;
}

uint32_t _hash_char(uint32_t hash_seed, Key * key) {
    register uint32_t result = 5381 ^ hash_seed;
    register unsigned char * ptr = (unsigned char *)key->shash;
    register int i;
    for (i = key->nhash; i > 0; --i) {
        result = 33 * result + *(ptr++);
    }
    return result;
}

uint32_t _hash_long(uint32_t hash_seed, Key * key) {
    return 5381 + (33 * (key->nhash ^ hash_seed));
}


#if 0
int main(int argc, char **argv)
{
    int hash_seeds[5] = { 4234 , 2123, 4434, 444, 12123};
    BloomFilter *bf = bloomfilter_Create(100000, 0.4,
                                         "/tmp/bf2", 10000000, O_RDWR, 0,
                                        hash_seeds, 5);

    Key key;
    char line[255];
    key.shash = line;

    if (!bf)
        goto error;

    bloomfilter_Print(bf);

    while (fgets(line, 255, stdin)) {
        line[strlen(line) - 1] = '\0';
        key.nhash = strlen(line);

        /*if (bloomfilter_Add(bf, &key)) {
            goto error;
            }*/
        if (bloomfilter_Test(bf, &key)) {
            printf("Found '%s'!\n", line);
        }
    }
    bloomfilter_Destroy(bf);
    return 0;

 error:
    fprintf(stderr, "ERROR: %s [%d]\n", strerror(errno), errno);
    return 255;
}
#endif
#endif
