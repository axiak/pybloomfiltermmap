#ifndef __MMAPBITARRAY_H
#define __MMAPBITARRAY_H 1
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

/* Types */
typedef uint32_t DTYPE;
typedef uint64_t BTYPE;

struct MmapBitArray {
    BTYPE bits;
    size_t size;
    size_t preamblesize;
    size_t bytes;
    size_t preamblebytes;
    const char * filename;
    DTYPE * vector;
    int32_t fd;
};

typedef struct MmapBitArray MBArray;


/* Constants */
enum {
    ONES = (DTYPE)-1,
    MBAMAGICSIZE = 9
};
#define MBAMAGIC "MBITARRAY"



/* Functions */

MBArray * mbarray_Create(BTYPE num_bits, const char * file, const char * header, int header_len, int oflag, int perms);

void mbarray_Destroy(MBArray * array);

int mbarray_ClearAll(MBArray * array);

int mbarray_Sync(MBArray * array);

int32_t mbarray_HeaderLen(MBArray * array);

char * mbarray_Header(char * dest, MBArray * array, int maxlen);

MBArray * mbarray_And(MBArray * dest, MBArray * array2);

MBArray * mbarray_Or(MBArray * dest, MBArray * array2);

MBArray * mbarray_Xor(MBArray * dest, MBArray * array2);

MBArray * mbarray_And_Ternary(MBArray * dest, MBArray * a, MBArray * b);

MBArray * mbarray_Or_Ternary(MBArray * dest, MBArray * a, MBArray * b);

MBArray * mbarray_Xor_Ternary(MBArray * dest, MBArray * a, MBArray * b);

MBArray * mbarray_Copy_Template(MBArray * src, char * filename, int perms);

int mbarray_Update(MBArray * array, char * data, int size);
/*MBArray * mbarray_Copy(MBarray * src, const char * filename);*/

int mbarray_FileSize(MBArray * array);

char * mbarray_CharData(MBArray * array);

static inline size_t _vector_offset(MBArray * array, BTYPE bit)
{
    return array->preamblesize + bit / (sizeof(DTYPE) << 3);
}
__attribute__((always_inline))


static inline size_t _vector_byte(BTYPE bit) {
    return 1 << (bit % (sizeof(DTYPE) << 3));
}
__attribute__((always_inline))


static inline int mbarray_Set(MBArray * array, BTYPE bit)
{
    if (bit > array->bits) {
        errno = EINVAL;
        return 1;
    }
    array->vector[_vector_offset(array, bit)] |= _vector_byte(bit);
    return 0;
}
__attribute__((always_inline))


static inline int mbarray_Clear(MBArray * array, BTYPE bit)
{
    if (bit > array->bits) {
        errno = EINVAL;
        return 1;
    }
    array->vector[_vector_offset(array, bit)] &= (ONES - _vector_byte(bit));
    return 0;
}
__attribute__((always_inline))


static inline int mbarray_Test(MBArray * array, BTYPE bit)
{
    if (bit > array->bits) {
        errno = EINVAL;
        return -1;
    }
    return ((array->vector[_vector_offset(array, bit)] & _vector_byte(bit)) != 0);
}
__attribute__((always_inline))


#endif
