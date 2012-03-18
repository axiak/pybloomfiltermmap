import sys
import unittest
import tempfile

import pybloomfilter

from tests import with_test_file

class SimpleTestCase(unittest.TestCase):
    @with_test_file
    def test_number(self, filename):
        bf = pybloomfilter.BloomFilter(100, 0.01, filename)
        bf.add(1234)
        self.assertEquals(1234 in bf, True)

    @with_test_file
    def test_string(self, filename):
        bf = pybloomfilter.BloomFilter(100, 0.01, filename)
        bf.add("test")
        self.assertEquals("test" in bf, True)

    @with_test_file
    def test_others(self, filename):
        bf = pybloomfilter.BloomFilter(100, 0.01, filename)
        for elem in (1.2, 2343L, (1, 2), object(), u'\u2131\u3184'):
            bf.add(elem)
            self.assertEquals(elem in bf, True)

    def test_number_nofile(self):
        bf = pybloomfilter.BloomFilter(100, 0.01)
        bf.add(1234)
        self.assertEquals(1234 in bf, True)

    def test_string_nofile(self):
        bf = pybloomfilter.BloomFilter(100, 0.01)
        bf.add("test")
        self.assertEquals("test" in bf, True)

    def test_others_nofile(self):
        bf = pybloomfilter.BloomFilter(100, 0.01)
        for elem in (1.2, 2343L, (1, 2), object(), u'\u2131\u3184'):
            bf.add(elem)
            self.assertEquals(elem in bf, True)

    #@unittest.skip("unfortunately large files cannot be tested on Travis")
    @with_test_file
    def _test_large_file(self, filename):
        bf = pybloomfilter.BloomFilter(400000000, 0.01, filename)
        bf.add(1234)
        self.assertEquals(1234 in bf, True)

def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(SimpleTestCase))
    return suite
