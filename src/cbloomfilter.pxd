
cdef extern from "primetester.h":
     long next_prime(long prime)

cdef extern from "mmapbitarray.h":
     ctypedef struct MBArray:
         long bits
         long size
         char * filename
         int fd

     MBArray * mbarray_ClearAll(MBArray * array)
     MBArray * mbarray_Sync(MBArray * array)
     MBArray * mbarray_And(MBArray * dest, MBArray * src)
     MBArray * mbarray_Or(MBArray * dest, MBArray * src)
     MBArray * mbarray_Xor(MBArray * dest, MBArray * src)
     MBArray * mbarray_And_Ternary(MBArray * dest, MBArray * a, MBArray * b)
     MBArray * mbarray_Or_Ternary(MBArray * dest, MBArray * a, MBArray * b)
     MBArray * mbarray_Xor_Ternary(MBArray * dest, MBArray * a, MBArray * b)
     int mbarray_Update(MBArray * array, char * data, int size)
     int mbarray_FileSize(MBArray * array)
     char * mbarray_CharData(MBArray * array)


cdef extern from "bloomfilter.h":
     ctypedef struct BloomFilter:
         long max_num_elem
         double error_rate
         int num_hashes
         long * hash_seeds
         MBArray * array
         unsigned char bf_version
         unsigned char count_correct
         unsigned long long elem_count

     ctypedef struct Key:
         long nhash
         char * shash

     BloomFilter * bloomfilter_Create_Mmap(long max_num_elem,
                                      double error_rate,
                                      char * fname, long num_bits,
                                      int oflags, int perms,
                                      int * hash_seeds, int num_hashes) nogil
     BloomFilter * bloomfilter_Create_Malloc(long max_num_elem,
                                      double error_rate,
                                      long num_bits,
                                      int * hash_seeds, int num_hashes) nogil
     void bloomfilter_Destroy(BloomFilter * bf) nogil
     int bloomfilter_Add(BloomFilter * bf, Key * key) nogil
     int bloomfilter_Test(BloomFilter * bf, Key * key) nogil
     int bloomfilter_Test_Add(BloomFilter * bf, Key * key) nogil
     int bloomfilter_Update(BloomFilter * bf, char * data, int size) nogil
     BloomFilter * bloomfilter_Copy_Template(BloomFilter * src, char * filename, int perms) nogil
