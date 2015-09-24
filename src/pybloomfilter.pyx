VERSION = (0, 3, 14)
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


cdef extern int errno

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

class IndeterminateCountError(ValueError):
    pass

cdef class BloomFilter:
    """
    The BloomFilter class implements a bloom filter that uses mmap'd files.
    For more information on what a bloom filter is, please read the Wikipedia article about it.
    """
    cdef cbloomfilter.BloomFilter * _bf
    cdef int _closed
    cdef int _in_memory
    cdef public ReadFile

    def __cinit__(self, capacity, error_rate, filename=None, perm=0755):
        cdef char * seeds
        cdef long long num_bits
        self._closed = 0
        self._in_memory = 0
        self.ReadFile = self.__class__.ReadFile
        mode = "rw+"
        if filename is NoConstruct:
            return

        if capacity is self.ReadFile:
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
                                                           mode,
                                                           perm,
                                                           NULL, 0)
                if self._bf is NULL:
                    raise ValueError("Invalid %s file: %s" %
                                     (self.__class__.__name__, filename))
            else:
                raise OSError(eno.ENOENT, '%s: %s' % (os.strerror(eno.ENOENT),
                                                      filename))
        else:
            # Make sure that if the filename is defined, that the
            # file exists
            if filename and os.path.exists(filename):
                os.unlink(filename)

            # For why we round down for determining the number of hashes:
            # http://corte.si/%2Fposts/code/bloom-filter-rules-of-thumb/index.html
            # "The number of hashes determines the number of bits that need to
            # be read to test for membership, the number of bits that need to be
            # written to add an element, and the amount of computation needed to
            # calculate hashes themselves. We may sometimes choose to use a less
            # than optimal number of hashes for performance reasons (especially
            # when we choose to round down when the calculated optimal number of
            # hashes is fractional)."

            assert(error_rate > 0.0 and error_rate < 1.0), "error_rate allowable range (0.0,1.0) %f" % (error_rate,)
            num_hashes = max(int(math.floor(math.log(1.0 / error_rate, 2.0))),1)
            bits_per_hash = int(math.ceil(
                    capacity * abs(math.log(error_rate)) /
                    (num_hashes * (math.log(2) ** 2))))

            # mininum bitvector of 128 bits
            num_bits = max(num_hashes * bits_per_hash,128)

            #print "k = %d  m = %d  n = %d   p ~= %.8f" % (
            #    num_hashes, num_bits, capacity,
            #    (1.0 - math.exp(- float(num_hashes) * float(capacity) / num_bits))
            #    ** num_hashes)

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
                self._in_memory = 1
                self._bf = cbloomfilter.bloomfilter_Create_Malloc(capacity,
                                                       error_rate,
                                                       num_bits,
                                                       <int *>seeds,
                                                       num_hashes)
            if self._bf is NULL:
                if filename:
                    raise OSError(errno, '%s: %s' % (os.strerror(errno),
                                                     filename))
                else:
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

    property num_bits:
        def __get__(self):
            self._assert_open()
            return self._bf.array.bits

    property name:
        def __get__(self):
            self._assert_open()
            if self._in_memory:
                raise NotImplementedError('Cannot access .name on an '
                                          'in-memory %s' %
                                          self.__class__.__name__)

            return self._bf.array.filename

    def fileno(self):
        self._assert_open()
        return self._bf.array.fd

    def __repr__(self):
        self._assert_open()
        my_name = self.__class__.__name__
        return '<%s capacity: %d, error: %0.3f, num_hashes: %d>' % (
            my_name, self._bf.max_num_elem, self._bf.error_rate,
            self._bf.num_hashes)

    def __str__(self):
        return self.__repr__()

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

    def contains_add(self, item):
        self._assert_open()
        cdef cbloomfilter.Key key
        if isinstance(item, str):
            key.shash = item
            key.nhash = len(item)
        else:
            key.shash = NULL
            key.nhash = hash(item)

        return cbloomfilter.bloomfilter_Test_Add(self._bf, &key) == 1

    def copy_template(self, filename, perm=0755):
        self._assert_open()
        cdef BloomFilter copy = BloomFilter(0, 0, NoConstruct)
        if os.path.exists(filename):
            os.unlink(filename)
        copy._bf = cbloomfilter.bloomfilter_Copy_Template(self._bf, filename, perm)
        return copy

    def copy(self, filename):
        self._assert_open()
        if self._in_memory:
            raise NotImplementedError('Cannot call .copy on an in-memory %s' %
                                      self.__class__.__name__)
        shutil.copy(self._bf.array.filename, filename)
        return self.__class__(self.ReadFile, 0.1, filename, perm=0)

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
            raise IndeterminateCountError("Length of %s object is unavailable "
                                          "after intersection or union called." %
                                          self.__class__.__name__)
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
        error = ValueError("The two %s objects are not the same type (hint, "
                           "use copy_template)" % self.__class__.__name__)
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

    @classmethod
    def from_base64(cls, filename, string, perm=0755):
        bfile_fp = os.open(filename, construct_mode('w+'), perm)
        os.write(bfile_fp, zlib.decompress(zlib.decompress(
            string.decode('base64')).decode('base64')))
        os.close(bfile_fp)
        return cls.open(filename)

    @classmethod
    def open(cls, filename):
        return cls(cls.ReadFile, 0.1, filename, 0)
