import os
import math
import string
import random
import unittest
import tempfile

import pybloomfilter


class TestAccuracyMixin(object):
    FILTER_SIZE = 1000

    def _gen_random_items(self, n, exclude=None):
        # Yield n unique random items; if an existing set is provided,
        # items already in that set will not be yielded.
        if exclude is not None:
            yielded = exclude
        else:
            yielded = set()

        yield_count = 0
        while yield_count < n:
            random_item = self._random_item()
            if random_item not in yielded:
                yield random_item
                yielded.add(random_item)
                yield_count += 1

    def test_false_pos_degredation(self):
        # we'll check 10% to 0.01%
        for error_rate in (0.1, 0.01, 0.001, 0.0001):
            bf = self._bf(error_rate)
            items_in_filter = set(
                self._gen_random_items(bf.capacity))

            items_in_filter_list = list(items_in_filter)
            n = 0
            chunk_count = 10
            chunk_size = int(math.ceil(float(bf.capacity) / chunk_count))
            print 'error_rate = %.4f' % error_rate
            print '    %6s %9s %s' % ('n', 'false_pos',
                                      'estimated error_rate')
            for i in range(chunk_count):
                chunk = items_in_filter_list[i*chunk_size:(i+1)*chunk_size]
                n += len(chunk)
                bf.update(chunk)
                pos_test_count = int(5 * 1.0 / error_rate)
                false_pos = len(filter(bf.__contains__,
                                       self._gen_random_items(
                                           pos_test_count, items_in_filter)))
                est_error_rate = float(false_pos) / pos_test_count
                print '    %6d %9d %.8f %s' % (
                    n, false_pos, est_error_rate,
                    '******' if est_error_rate > error_rate else '')

    def test_accuracy(self):
        print '\n%14s\t%14s\t%17s' % ('pos_test_count', 'false_pos_rate',
                                      'error_rate_target')
        # we'll check 10% to 0.01%
        for error_rate in (0.1, 0.01, 0.001, 0.0001):
            bf = self._bf(error_rate)
            items_in_filter = set(
                self._gen_random_items(bf.capacity))
            bf.update(items_in_filter)

            # sanity check
            self.assertEqual(bf.capacity, len(items_in_filter))

            false_neg = len(items_in_filter) - \
                len(filter(bf.__contains__, items_in_filter))

            pos_test_count = int(10 * (1.0 / error_rate))
            false_pos = len(filter(bf.__contains__, self._gen_random_items(
                pos_test_count, items_in_filter)))
            false_pos = 0
            for test in self._gen_random_items(pos_test_count,
                                               items_in_filter):
                if test in bf:
                    false_pos += 1

            false_pos_rate = float(false_pos) / pos_test_count
            false_neg_rate = float(false_neg) / len(items_in_filter)
            error_rate_target = error_rate * 2  # cut it some slack

            print '%14d\t%14f\t%17f' % (pos_test_count, false_pos_rate,
                                        error_rate_target)
            self.assertTrue(
                false_pos_rate <= error_rate_target,
                "false_pos: %r / %r = %r > %r" % (
                    false_pos, pos_test_count,
                    false_pos / float(pos_test_count), error_rate_target))
            self.assertEqual(false_neg_rate, 0.0,
                             "false negative rate is nonzero: %0.6f" %
                             (false_neg_rate,))
            del bf


class StringAccuracyMallocTestCase(unittest.TestCase, TestAccuracyMixin):
    CHARS = string.lowercase + string.uppercase
    STR_LEN = 10

    def _random_item(self):
        return ''.join(random.choice(self.CHARS)
                       for _ in xrange(self.STR_LEN))

    def _bf(self, error_rate):
        return pybloomfilter.BloomFilter(self.FILTER_SIZE, error_rate)


class StringAccuracyMmapTestCase(unittest.TestCase, TestAccuracyMixin):
    CHARS = string.lowercase + string.uppercase
    STR_LEN = 10

    def setUp(self):
        self.temp_file = tempfile.NamedTemporaryFile(suffix='.bloom',
                                                     delete=False)

    def tearDown(self):
        os.unlink(self.temp_file.name)

    def _random_item(self):
        return ''.join(random.choice(self.CHARS)
                       for _ in xrange(self.STR_LEN))

    def _bf(self, error_rate):
        return pybloomfilter.BloomFilter(self.FILTER_SIZE, error_rate,
                                         self.temp_file.name)


class IntegerAccuracyMallocTestCase(unittest.TestCase, TestAccuracyMixin):

    def _random_item(self):
        return random.randint(-2**31, 2**31)

    def _bf(self, error_rate):
        return pybloomfilter.BloomFilter(self.FILTER_SIZE, error_rate)


class IntegerAccuracyMmapTestCase(unittest.TestCase, TestAccuracyMixin):

    def setUp(self):
        self.temp_file = tempfile.NamedTemporaryFile(suffix='.bloom',
                                                     delete=False)

    def tearDown(self):
        os.unlink(self.temp_file.name)

    def _random_item(self):
        return random.randint(-2**31, 2**31)

    def _bf(self, error_rate):
        return pybloomfilter.BloomFilter(self.FILTER_SIZE, error_rate,
                                         self.temp_file.name)


def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(StringAccuracyMmapTestCase))
    suite.addTest(unittest.makeSuite(StringAccuracyMallocTestCase))
    suite.addTest(unittest.makeSuite(IntegerAccuracyMmapTestCase))
    suite.addTest(unittest.makeSuite(IntegerAccuracyMallocTestCase))
    return suite
