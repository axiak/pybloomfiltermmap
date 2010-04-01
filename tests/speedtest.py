import os
import tempfile
import time
import timeit

import pybloomfilter

tempfiles = []

ERROR_RATE = 0.1

#def get_and_add_words(Creator, wordlist):
def get_and_add_words(Creator, wordlist):
    bf = Creator(len(wordlist), ERROR_RATE)
    for word in wordlist:
        bf.add(word)
    return bf

def check_words(bf, wordlist):
    for word in wordlist:
        word in bf

def test_errors(bf, correct_wordlist, test_wordlist):
    errors = [0, 0]
    for word in test_wordlist:
        if word in bf:
            if word not in correct_wordlist:
                errors[0] += 1
        else:
            if word in correct_wordlist:
                errors[1] += 1
    print '%0.2f%% positive %0.2f%% negative' % (
        errors[0] / float(len(correct_wordlist)) * 100,
        errors[1] / float(len(correct_wordlist)) * 100)

def create_word_list(filename):
    f = open(filename, 'r')
    words_set = set()
    for line in f:
        line = line.strip().lower()
        if line:
            words_set.add(line)
    f.close()
    return words_set

def create_cbloomfilter(*args):
    args = list(args)
    f = tempfile.NamedTemporaryFile()
    tempfiles.append(f)
    os.unlink(f.name)
    args.append(f.name)
    return pybloomfilter.BloomFilter(*tuple(args))

creators = [create_cbloomfilter]
try:
    import pybloom
except ImportError:
    pass
else:
    creators.append(pybloom.BloomFilter)

def run_test():
    dict_wordlist = create_word_list('words')
    test_wordlist = create_word_list('testwords')
    NUM = 10

    for creator in creators:
        start = time.time()
        if NUM:
            t = timeit.Timer(lambda : get_and_add_words(creator, dict_wordlist))
            print "%s took %0.2f s/run" % (
                creator,
                t.timeit(NUM) / float(NUM))
        bf = get_and_add_words(creator, dict_wordlist)

        if NUM:
            t = timeit.Timer(lambda : check_words(bf, test_wordlist))
            print "%s took %0.2f s/run" % (
                creator,
                t.timeit(NUM) / float(NUM))

        raw_input()

        test_errors(bf, dict_wordlist, test_wordlist)

if __name__ == "__main__":
    run_test()
