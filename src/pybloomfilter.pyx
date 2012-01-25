VERSION = (0, 3, 2)
AUTHOR = "Michael Axiak"

__VERSION__ = VERSION


cimport cbloomfilter
cimport cpython

import random
import os
import math
import errno as eno
import array
import zlib
import shutil

global errno

cdef construct_mode(mode):
    result = os.O_RDONLY
    if 'w' in mode:
        result |= os.O_RDWR
    if 'b' in mode and hasattr(os, 'O_BINARY'):
        result |= os.O_BINARY
    if mode.endswith('+'):
        result |= os.O_CREAT
    return result

cdef NoConstruct = object()
cdef ReadFile = object()

def bf_from_base64(filename, string, perm=0755):
    bfile = open(filename, 'w+', perm)
    bfile.write(zlib.decompress(zlib.decompress(string.decode('base64')).decode('base64')))
    bfile.close()
    return BloomFilter.open(filename)

def bf_from_file(filename):
    return BloomFilter(ReadFile, 0.1, filename, 0)

class IndeterminateCountError(ValueError):
    pass

cdef class BloomFilter:
    """
    The BloomFilter class implements a bloom filter that uses mmap'd files.
    For more information on what a bloom filter is, please read the Wikipedia article about it.
    """
    cdef cbloomfilter.BloomFilter * _bf
    cdef int _closed

    def __cinit__(self, capacity, error_rate, filename=None, perm=0755):
        cdef char * seeds
        cdef long long num_bits
        _closed = 0
        mode = "rw+"
        if filename is NoConstruct:
            return

        if capacity is ReadFile:
            mode = "rw"
            capacity = 0
            if not os.path.exists(filename):
                raise OSError("File %s not found" % filename)

            if not os.access(filename, os.O_RDWR):
                raise OSError("Insufficient permissions for file %s" % filename)

        mode = construct_mode(mode)


        if not mode & os.O_CREAT:
            if os.path.exists(filename):
                self._bf = cbloomfilter.bloomfilter_Create_Mmap(capacity,
                                                           error_rate,
                                                           filename,
                                                           0,
                                                           os.O_RDWR,
                                                           perm,
                                                           NULL, 0)
                if self._bf is NULL:
                    raise ValueError("Invalid Bloomfilter file: %s" % filename)
            else:
                raise OSError(eno.ENOENT, '%s: %s' % (os.strerror(eno.ENOENT),
                                                      filename))
        else:
            # Make sure that if the filename is defined, that the
            # file exists
            if filename and os.path.exists(filename):
                os.unlink(filename)

            num_hashes = int(math.ceil(math.log(1.0 / error_rate, 2.0)))
            bits_per_hash = int(math.ceil(
                    (2.0 * capacity * abs(math.log(error_rate))) /
                    (num_hashes * (math.log(2) ** 2))))

            num_bits = num_hashes * bits_per_hash
            #num_bits = cbloomfilter.next_prime(num_bits)
            #num_hashes = int(math.ceil(math.log(2.0) * num_bits / capacity))

            hash_seeds = array.array('I')
            hash_seeds.extend([random.getrandbits(32) for i in range(num_hashes)])
            test = hash_seeds.tostring()
            seeds = test

            # If a filename is provided, we should make a mmap-file
            # backed bloom filter. Otherwise, it will be malloc
            if filename:
                self._bf = cbloomfilter.bloomfilter_Create_Mmap(capacity,
                                                       error_rate,
                                                       filename,
                                                       num_bits,
                                                       mode,
                                                       perm,
                                                       <int *>seeds,
                                                       num_hashes)
            else:
                self._bf = cbloomfilter.bloomfilter_Create_Malloc(capacity,
                                                       error_rate,
                                                       num_bits,
                                                       <int *>seeds,
                                                       num_hashes)
            if self._bf is NULL:
                cpython.PyErr_NoMemory()

    def __dealloc__(self):
        cbloomfilter.bloomfilter_Destroy(self._bf)
        self._bf = NULL

    property hash_seeds:
        def __get__(self):
            self._assert_open()
            result = array.array('I')
            result.fromstring((<char *>self._bf.hash_seeds)[:4 * self.num_hashes])
            return result

    property capacity:
        def __get__(self):
            self._assert_open()
            return self._bf.max_num_elem

    property error_rate:
        def __get__(self):
            self._assert_open()
            return self._bf.error_rate

    property num_hashes:
        def __get__(self):
            self._assert_open()
            return self._bf.num_hashes

    property num_slices:
        def __get__(self):
            self._assert_open()
            return self._bf.num_hashes

    property num_bits:
        def __get__(self):
            self._assert_open()
            return self._bf.array.bits

    property name:
        def __get__(self):
            self._assert_open()
            return self._bf.array.filename

    def fileno(self):
        self._assert_open()
        return self._bf.array.fd

    def __repr__(self):
        self._assert_open()
        return '<BloomFilter capacity: %d, error: %0.3f, num_hashes: %d>' % (
            self._bf.max_num_elem, self._bf.error_rate, self._bf.num_hashes)

    def __str__(self):
        return __repr__(self)

    def sync(self):
        self._assert_open()
        cbloomfilter.mbarray_Sync(self._bf.array)

    def clear_all(self):
        self._assert_open()
        cbloomfilter.mbarray_ClearAll(self._bf.array)

    def __contains__(self, item):
        self._assert_open()
        cdef cbloomfilter.Key key
        if isinstance(item, str):
            key.shash = item
            key.nhash = len(item)
        else:
            key.shash = NULL
            key.nhash = hash(item)
        return cbloomfilter.bloomfilter_Test(self._bf, &key) == 1

    def copy_template(self, filename, perm=0755):
        self._assert_open()
        cdef BloomFilter copy = BloomFilter(0, 0, NoConstruct)
        if os.path.exists(filename):
            os.unlink(filename)
        copy._bf = cbloomfilter.bloomfilter_Copy_Template(self._bf, filename, perm)
        return copy

    def copy(self, filename):
        self._assert_open()
        shutil.copy(self._bf.array.filename, filename)
        return BloomFilter(self._bf.max_num_elem, self._bf.error_rate, filename, mode="rw", perm=0)

    def add(self, item):
        self._assert_open()
        cdef cbloomfilter.Key key
        if isinstance(item, str):
            key.shash = item
            key.nhash = len(item)
        else:
            key.shash = NULL
            key.nhash = hash(item)

        result = cbloomfilter.bloomfilter_Add(self._bf, &key)
        if result == 2:
            raise RuntimeError("Some problem occured while trying to add key.")
        return bool(result)

    def update(self, iterable):
        self._assert_open()
        for item in iterable:
            self.add(item)

    def __len__(self):
        self._assert_open()
        if not self._bf.count_correct:
            raise IndeterminateCountError("Length of BloomFilter object is unavailable after intersection or union called.")
        return self._bf.elem_count

    def close(self):
        if self._closed == 0:
            self._closed = 1
            cbloomfilter.bloomfilter_Destroy(self._bf)
            self._bf = NULL

    def __ior__(self, BloomFilter other):
        self._assert_open()
        self._assert_comparable(other)
        cbloomfilter.mbarray_Or(self._bf.array, other._bf.array)
        self._bf.count_correct = 0
        return self

    def union(self, BloomFilter other):
        self._assert_open()
        other._assert_open()
        self._assert_comparable(other)
        cbloomfilter.mbarray_Or(self._bf.array, other._bf.array)
        self._bf.count_correct = 0
        return self

    def __iand__(self, BloomFilter other):
        self._assert_open()
        other._assert_open()
        self._assert_comparable(other)
        cbloomfilter.mbarray_And(self._bf.array, other._bf.array)
        self._bf.count_correct = 0
        return self

    def intersection(self, BloomFilter other):
        self._assert_open()
        other._assert_open()
        self._assert_comparable(other)
        cbloomfilter.mbarray_And(self._bf.array, other._bf.array)
        self._bf.count_correct = 0
        return self

    def _assert_open(self):
        if self._closed != 0:
            raise ValueError("I/O operation on closed file")

    def _assert_comparable(self, BloomFilter other):
        error = ValueError("The two BloomFilter objects are not the same type (hint, use copy_template)")
        if self._bf.array.bits != other._bf.array.bits:
            raise error
        if self.hash_seeds != other.hash_seeds:
            raise error
        return

    def to_base64(self):
        self._assert_open()
        bfile = open(self.name, 'r')
        result = zlib.compress(zlib.compress(bfile.read(), 9).encode('base64')).encode('base64')
        bfile.close()
        return result

    from_base64 = staticmethod(bf_from_base64)
    open = staticmethod(bf_from_file)
