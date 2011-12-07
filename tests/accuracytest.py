#! /usr/bin/env python

import sys
import os
import tempfile
import pybloomfilter

WORDS_FILE = os.path.join(os.path.dirname(__file__), 'words')
TEST_WORDS = os.path.join(os.path.dirname(__file__), 'testwords')

def main():
    global pybloomfilter

    if len(sys.argv) > 1 and sys.argv[1].lower() == '-pybloom':
        import pybloom
        pybloomfilter = pybloom

    with open(WORDS_FILE) as base_file:
        with open(TEST_WORDS) as test_file:
            base_words = set(base_file)
            test_words = set(test_file)
            correct_overlap = len(base_words & test_words)
            num_test_words = len(test_words)
            number_words = len(base_words)

    for error_rate in (0.01, 0.001, 0.0001):
        test_errors(error_rate, number_words, correct_overlap, num_test_words)


def test_errors(error_rate, filter_size, correct_overlap, num_test_words):
    bloom_file = tempfile.NamedTemporaryFile()
    try:
        bf = pybloomfilter.BloomFilter(filter_size, error_rate, bloom_file.name)
    except TypeError:
        bf = pybloomfilter.BloomFilter(filter_size, error_rate)

    with open(WORDS_FILE) as source_file:
        with open(TEST_WORDS) as test_file:
            run_test(bf, source_file, test_file, correct_overlap, num_test_words, error_rate)

    #os.unlink(bloom_file.name)


def run_test(bf, source_file, test_file, correct_overlap, num_test_words, error_rate):
    for word in source_file:
        bf.add(word.rstrip())

    positive_matches = sum(1 for word in test_file
                           if word.rstrip() in bf)


    actual_error_rate = float(positive_matches - correct_overlap) / correct_overlap

    print "Specified: %f; Measured: %f; num_hashes: %d, num_bits: %d" % (
        error_rate,
        actual_error_rate,
        bf.num_slices,
        bf.num_bits,
        )



if __name__ == '__main__':
    main()
