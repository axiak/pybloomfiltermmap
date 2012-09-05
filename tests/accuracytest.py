import random
import string
import unittest
import tempfile

import pybloomfilter

from tests import with_test_file

class AccuracyTestCase(unittest.TestCase):
    def test_strings(self):
        random_strings = list(set(''.join(random.choice(string.lowercase + string.uppercase)
                                          for _ in range(6)) for _ in range(10000)))
        random.shuffle(random_strings)

        for accuracy in (0.1, 0.01, 0.001):
            bf = pybloomfilter.BloomFilter(8000, accuracy)
            bf.update(random_strings[:8000])
            false_pos, false_neg = 0, 0
            for test in random_strings[8000:10000]:
                if test in bf:
                    false_pos += 1
            for test in random_strings[6000:8000]:
                if test not in bf:
                    false_neg += 1
            false_pos_rate = false_pos / 2000.
            false_neg_rate = false_neg / 2000.
            self.assertTrue(false_pos_rate <= accuracy*2, "accuracy fail: %0.4f > %0.4f" % (false_pos_rate, accuracy))
            self.assertEqual(false_neg_rate, 0.0, "false negative rate is nonzero: %0.4f" % (false_neg_rate,))
            del bf
            print false_pos_rate, accuracy

    def test_ints(self):
        random_strings = list(range(10000))

        for accuracy in (0.1, 0.01, 0.001):
            bf = pybloomfilter.BloomFilter(8000, accuracy)
            bf.update(random_strings[:8000])
            false_pos, false_neg = 0, 0
            for test in random_strings[8000:10000]:
                if test in bf:
                    false_pos += 1
            for test in random_strings[6000:8000]:
                if test not in bf:
                    false_neg += 1
            false_pos_rate = false_pos / 2000.
            false_neg_rate = false_neg / 2000.
            self.assertTrue(false_pos_rate <= accuracy*7, "accuracy fail: %0.4f > %0.4f" % (false_pos_rate, accuracy))
            self.assertEqual(false_neg_rate, 0.0, "false negative rate is nonzero: %0.4f" % (false_neg_rate,))
            print false_pos_rate, accuracy
            del bf

def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(AccuracyTestCase))
    return suite
