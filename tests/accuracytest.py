import string
import random
import unittest

import pybloomfilter


class AccuracyTestCase(unittest.TestCase):
    def test_strings(self):
        count = 80000
        count_in_filter = count * 3 / 5
        count_not_in_filter = count * 2 / 5
        # Construct "count" unique random strings
        chars = string.lowercase + string.uppercase
        random_strings = set()
        while len(random_strings) < count:
            random_strings.add(''.join(
                random.choice(chars) for _ in xrange(64)))

        random_strings = list(random_strings)

        print '\n%r\t%r' % ('false_pos_rate', 'error_rate_target')
        for accuracy in (0.1, 0.01, 0.001):
            bf = pybloomfilter.BloomFilter(count_in_filter, accuracy)
            bf.update(random_strings[:count_in_filter])
            false_pos, false_neg = 0, 0
            for test in random_strings[count_in_filter:count]:
                if test in bf:
                    false_pos += 1
            for test in random_strings[:count_in_filter]:
                if test not in bf:
                    false_neg += 1
            false_pos_rate = false_pos / float(count_not_in_filter)
            false_neg_rate = false_neg / float(count_in_filter)
            error_rate_target = accuracy * 2  # cut it some slack
            self.assertTrue(
                false_pos_rate <= error_rate_target,
                "false_pos: %r / %r = %r > %r" % (
                    false_pos, count_not_in_filter,
                    false_pos / float(count_not_in_filter), error_rate_target))
            self.assertEqual(false_neg_rate, 0.0,
                             "false negative rate is nonzero: %0.6f" %
                             (false_neg_rate,))
            del bf
            print '%r\t\t%r' % (false_pos_rate, error_rate_target)

    def test_ints(self):
        count = 80000
        count_in_filter = count * 3 / 5
        count_not_in_filter = count * 2 / 5
        # Construct "count" unique random integers
        random_ints = set()
        while len(random_ints) < count:
            random_ints.add(random.randint(-2**31, 2**31))

        random_ints = list(random_ints)

        print '\n%r\t%r' % ('false_pos_rate', 'error_rate_target')
        for accuracy in (0.1, 0.01, 0.001):
            bf = pybloomfilter.BloomFilter(count_in_filter, accuracy)
            bf.update(random_ints[:count_in_filter])
            false_pos, false_neg = 0, 0
            for test in random_ints[count_in_filter:count]:
                if test in bf:
                    false_pos += 1
            for test in random_ints[:count_in_filter]:
                if test not in bf:
                    false_neg += 1
            false_pos_rate = false_pos / float(count_not_in_filter)
            false_neg_rate = false_neg / float(count_in_filter)
            error_rate_target = accuracy * 3  # cut it some slack
            self.assertTrue(
                false_pos_rate <= error_rate_target,
                "false_pos: %r / %r = %r > %r" % (
                    false_pos, count_not_in_filter,
                    false_pos / float(count_not_in_filter), error_rate_target))
            self.assertEqual(false_neg_rate, 0.0,
                             "false negative rate is nonzero: %0.6f" %
                             (false_neg_rate,))
            del bf
            print '%r\t\t%r' % (false_pos_rate, error_rate_target)


def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(AccuracyTestCase))
    return suite
