#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include "mmapbitarray.h"

/* Private helpers */
static inline uint64_t _filesize(int fd);
static inline int _valid_magic(int fd);
int _initialize_file(int fd, size_t end, BTYPE num_bits, const char * header, int32_t header_len);
uint64_t _get_num_bits(int fd);
static inline size_t _mmap_size(MBArray * array);
/*    __attribute__((always_inline));*/

static inline int _assert_comparable(MBArray * array1, MBArray * array2);
/*    __attribute__((always_inline));;*/

MBArray * mbarray_Create_Malloc(BTYPE num_bits)
{
    // Try to allocate space for a MBArray struct
    errno = 0;
    MBArray * array = (MBArray *)malloc(sizeof(MBArray));

    // And ensure that it was constructed properly
    if (!array || errno) {
        return NULL;
    }

    // Since we're not using a real mmap file for this instance,
    // we can get away with setting a bunch of the internal vars
    // to be reasonable default values
    array->filename      = NULL;
    array->vector        = NULL;
	array->fd            = 0;
	array->preamblesize  = 0;
	array->preamblebytes = 0;

    // This is how many DTYPEs there are, and how many bytes there
    // are in this particular structure. As well as the number of 
    // bits
    array->size  = (size_t)ceil((double)num_bits / sizeof(DTYPE) / 8.0);
    array->bytes = (size_t)ceil((double)num_bits / 8.0);
    array->bits  = num_bits;

    // Now try to allocate enough space for our array
    errno = 0;
    array->vector = (DTYPE *)calloc(array->bytes, 1);
    if (errno || !array->vector) {
        mbarray_Destroy(array);
        return NULL;
    }

    return array;
}

MBArray * mbarray_Create_Mmap(BTYPE num_bits, const char * file, const char * header, int32_t header_len, int oflag, int perms)
{
    errno = 0;
    MBArray * array = (MBArray *)malloc(sizeof(MBArray));
    uint64_t filesize;
    int32_t fheaderlen;

    if (!array || errno) {
        return NULL;
    }

    array->filename = NULL;
    array->vector = NULL;
    errno = 0;
    array->fd = open(file, oflag, perms);

    if (array->fd < 0) {
        errno = EINVAL;
        mbarray_Destroy(array);
        return NULL;
    }

    fheaderlen = mbarray_HeaderLen(array);
    errno = 0;
    if (fheaderlen >= 0 && !(oflag & O_CREAT) && fheaderlen != header_len) {
        errno = EINVAL;
        mbarray_Destroy(array);
        return NULL;
    }
    else if (fheaderlen >= 0) {
        header_len = fheaderlen;
    }

    array->preamblebytes = MBAMAGICSIZE + sizeof(BTYPE) + sizeof(header_len) + header_len;

    /* This size is using 256-byte alignment so that we can use pretty much any base 2 data type */
    array->preamblesize = ((int)ceil((double)array->preamblebytes / 256.0) * 256) / sizeof(DTYPE);
    array->preamblebytes = array->preamblesize * (sizeof(DTYPE));

    if (errno) {
        mbarray_Destroy(array);
        return NULL;
    }

    filesize = _filesize(array->fd);
    if (filesize > 50 && !num_bits) {
        num_bits = _get_num_bits(array->fd);
    }
    array->size = (size_t)ceil((double)num_bits / sizeof(DTYPE) / 8.0);
    array->bytes = (size_t)ceil((double)num_bits / 8.0);

    if (filesize == 0xffffffffffffffff) {
        mbarray_Destroy(array);
        return NULL;
    }
    else if (filesize && !_valid_magic(array->fd)) {
        errno = EINVAL;
        mbarray_Destroy(array);
        return NULL;
    }
    else if (filesize && filesize < (array->bytes + array->preamblebytes - 1)) {
        errno = EINVAL;
        mbarray_Destroy(array);
        return NULL;
    }
    else if (!filesize) {
        if (!(oflag & O_CREAT) || (!num_bits) || _initialize_file(array->fd, array->bytes + array->preamblebytes - 1, num_bits, header, header_len)) {
            if (!errno) {
                errno = ENOENT;
            }
            mbarray_Destroy(array);
            return NULL;
        }
    }
    else {
        if (!num_bits) {
            num_bits = _get_num_bits(array->fd);
            array->size = (size_t)ceil((double)num_bits / sizeof(DTYPE) / 8.0);
            array->bytes = (size_t)ceil((double)num_bits / 8.0);
        }
        else if (_get_num_bits(array->fd) != num_bits) {
            mbarray_Destroy(array);
            errno = EINVAL;
            return NULL;
        }
    }

    errno = 0;
    array->vector = (DTYPE *)mmap(NULL,
                                  _mmap_size(array),
                                  (oflag & O_RDWR) ? (PROT_READ | PROT_WRITE) : PROT_READ,
                                  MAP_SHARED, 
                                  array->fd,
                                  0);
    if (errno || !array->vector) {
        mbarray_Destroy(array);
        return NULL;
    }
    array->filename = (char *)malloc(strlen(file) + 1);
    if (!array->filename) {
        mbarray_Destroy(array);
        return NULL;
    }
    strcpy((char *)array->filename, file);
    array->bits = num_bits;
    return array;
}

void mbarray_Destroy(MBArray * array)
{
    if (array != NULL) {
        if (array->vector != NULL) {
            if (array->filename == NULL) {
                // This is the case where we initialized the vector
                // with malloc, and not mmap. As such, be free!
				free((void*)array->vector);
				array->vector = NULL;
            } else {
                if (munmap(array->vector, _mmap_size(array))) {
                    fprintf(stderr, "Unable to close mmap!\n");
                }
                if (array->fd >= 0) {
                    fsync(array->fd);
                    close(array->fd);
                    array->fd = -1;
                }
                array->vector = NULL;
            }
        }
        if (array->filename) {
            free((void *)array->filename);
            array->filename = NULL;
        }
        free(array);
    }
}

int32_t mbarray_HeaderLen(MBArray * array)
{
    int32_t header_len;
    errno = 0;
    if (pread(array->fd, &header_len, sizeof(header_len), MBAMAGICSIZE + sizeof(BTYPE)) != sizeof(header_len)) {
        return -1;
    }
    return header_len;
}

char * mbarray_Header(char * dest, MBArray * array, int maxlen)
{
    int32_t header_len = mbarray_HeaderLen(array);
    int readnum = (maxlen < header_len) ? (maxlen) : header_len;

    errno = 0;

    if (pread(array->fd,
              dest,
              readnum,
              MBAMAGICSIZE + sizeof(BTYPE) + sizeof(int32_t)) != readnum) {
        return NULL;
    }
    return dest;
}


int mbarray_Sync(MBArray * array)
{
    if (!array || !array->vector) {
        errno = EINVAL;
        return 1;
    }
    if (msync(array->vector, _mmap_size(array), MS_ASYNC)) {
        return 1;
    }
    return 0;
}


int mbarray_ClearAll(MBArray * array)
{
    if (!array || !array->vector) {
        errno = EINVAL;
        return 1;
    }
    memset((void *)(array->vector + array->preamblesize), 0, sizeof(DTYPE) * array->size);
    return 0;
}


MBArray * mbarray_And(MBArray * dest, MBArray * array2)
{
    register int i;
    if (_assert_comparable(dest, array2))
        return NULL;

    for (i = 0; i < dest->size + dest->preamblesize; i++) {
        dest->vector[i] &= array2->vector[i];
    }
    return dest;
}


MBArray * mbarray_Or(MBArray * dest, MBArray * array2)
{
    register int i;
    if (_assert_comparable(dest, array2))
        return NULL;
    for (i = 0; i < dest->size + dest->preamblesize; i++) {
        dest->vector[i] |= array2->vector[i];
    }
    return dest;
}


MBArray * mbarray_Xor(MBArray * dest, MBArray * array2)
{
    register int i;
    if (_assert_comparable(dest, array2))
        return NULL;

    for (i = 0; i < dest->size + dest->preamblesize; i++) {
        dest->vector[i] ^= array2->vector[i];
    }
    return dest;
}


MBArray * mbarray_And_Ternary(MBArray * dest, MBArray * a, MBArray * b)
{
    register int i;
    if (_assert_comparable(a, b) || _assert_comparable(dest, b))
        return NULL;

    for (i = 0; i < a->size + a->preamblesize; i++) {
        dest->vector[i] = a->vector[i] & b->vector[i];
    }
    return dest;
}

MBArray * mbarray_Or_Ternary(MBArray * dest, MBArray * a, MBArray * b)
{
    register int i;
    if (_assert_comparable(a, b) || _assert_comparable(dest, b))
        return NULL;

    for (i = 0; i < a->size + a->preamblesize; i++) {
        dest->vector[i] = a->vector[i] | b->vector[i];
    }
    return dest;
}

MBArray * mbarray_Xor_Ternary(MBArray * dest, MBArray * a, MBArray * b)
{
    register int i;
    if (_assert_comparable(a, b) || _assert_comparable(dest, b))
        return NULL;

    for (i = 0; i < a->size + a->preamblesize; i++) {
        dest->vector[i] = a->vector[i] ^ b->vector[i];
    }
    return dest;
}


MBArray * mbarray_Copy_Template(MBArray * src, char * filename, int perms)
{
    int header_len = mbarray_HeaderLen(src);
    char * header;

    if (header_len < 0) {
        return NULL;
    }

    if (!strcmp(filename, src->filename)) {
        errno = EINVAL;
        return NULL;
    }

    header = (char *)malloc(header_len + 1);
    if (header == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    if (mbarray_Header(header, src, header_len) == NULL) {
        free(header);
        return NULL;
    }

    return mbarray_Create_Mmap(
                          src->bits,
                          filename,
                          header,
                          header_len,
                          O_CREAT | O_RDWR,
                          perms);
}


/*MBArray * mbarray_Copy(MBarray * src, const char * filename);*/
uint64_t mbarray_FileSize(MBArray * array)
{
    return _filesize(array->fd);
}

char * mbarray_CharData(MBArray * array)
{
    return (char *)array->vector;
}


int mbarray_Update(MBArray * array, char * data, int size)
{
    memcpy(array->vector, data, size);
    array->bits = _get_num_bits(array->fd);
    array->size = (size_t)ceil((double)array->bits / sizeof(DTYPE) / 8.0);
    array->bytes = (size_t)ceil((double)array->bits / 8.0);
    return 0;
}

static inline int _assert_comparable(MBArray * array1, MBArray * array2)
{
    errno = EINVAL;
    if (array1->preamblebytes != array2->preamblebytes) {
        return 1;
    }

    if (memcmp((char *)array1->vector, (char *)array2->vector, array1->preamblebytes)) {
        return 1;
    }

    return 0;
}
__attribute__((always_inline))


static inline size_t _mmap_size(MBArray * array)
{
    return array->bytes + array->preamblebytes;
}
__attribute__((always_inline))


static inline int _valid_magic(int fd)
{
    size_t nbytes;
    char buffer[MBAMAGICSIZE + 1];

    nbytes = pread(fd, buffer, MBAMAGICSIZE, 0);
    if (errno || nbytes != MBAMAGICSIZE || strncmp(MBAMAGIC, buffer, MBAMAGICSIZE)) {
        return 0;
    }
    else {
        return 1;
    }
}

static inline uint64_t _filesize(int fd)
{
    struct stat buffer;
    int status;
    status = fstat(fd, &buffer);
    if (status || errno) {
        return (uint64_t)0xffffffffffffffff;
    }

    return (uint64_t)buffer.st_size;
}

uint64_t _get_num_bits(int fd) {
    uint64_t num_bits;
    errno = 0;
    if (pread(fd, &num_bits, sizeof(uint64_t), MBAMAGICSIZE) != sizeof(uint64_t)) {
        return 0;
    }
    return num_bits;
}

int _initialize_file(int fd, size_t end, BTYPE num_bits, const char * header, int32_t header_len)
{
    unsigned char zero = 0;
    errno = 0;
    lseek(fd, 0, SEEK_SET);
    if (write(fd, MBAMAGIC, MBAMAGICSIZE) != MBAMAGICSIZE) {
        return 1;
    }
    if (write(fd, &num_bits, sizeof(BTYPE)) != sizeof(BTYPE)) {
        return 1;
    }
    if (write(fd, &header_len, sizeof(header_len)) != sizeof(header_len)) {
        return 1;
    }
    if (header_len) {
        if (write(fd, header, header_len) != header_len) {
            return 1;
        }
    }

    lseek(fd, end, SEEK_SET);
    if (write(fd, &zero, 1) != 1) {
        return 1;
    }

    if (errno) {
        return 1;
    }
    return 0;
}



#ifdef MBACREATE
int main(int argc, char ** argv)
{
    MBArray * array;
    if (argc < 3) {
        fprintf(stderr, "Usage: %s FILENAME SIZE\nCreate new mmap'd array file.\n", argv[0]);
        return 1;
    }

    array = mbarray_Create_Mmap(
                           atol(argv[2]),
                           argv[1],
                           "",
                           0,
                           O_RDWR | O_CREAT,
                           0777);
    if (!array)
        goto error;
    mbarray_ClearAll(array);
    mbarray_Destroy(array);
    return 0;
    error:
    fprintf(stderr, "Error: %s [%d]\n", strerror(errno), errno);
    return 255;
}
#endif

#ifdef MBAQUERY
int main(int argc, char ** argv)
{
    BTYPE bit;
    int value;
    MBArray * array;
    int i;
    if (argc < 3) {
        fprintf(stderr, "Usage: %s FILE BIT [VALUE]\nValue is either 0 or 1 and will define a set/clear operation.\n", argv[0]);
        return 255;
    }

    /* Open file */
    array = mbarray_Create_Mmap(
                           0,
                           argv[1],
                           "",
                           0,
                           O_RDWR,
                           0);
    if (!array)
        goto error;

    bit = atol(argv[2]);

    if (argc > 3) {
        value = atol(argv[3]);
        if (value) {
            if (mbarray_Set(array, bit))
                goto error;
        }
        else {
            if (mbarray_Clear(array, bit))
                goto error;
        }
    }

    for (i = 0; i < array->bits; i++) {
        mbarray_Set(array, i);
        mbarray_Test(array, i);
    }
    getc(stdin);
    bit = 1 - mbarray_Test(array, bit);
    mbarray_Destroy(array);
    return bit;
    error:
    fprintf(stderr, "Error: %s [%d]\n", strerror(errno), errno);
    return 255;
}
#endif
