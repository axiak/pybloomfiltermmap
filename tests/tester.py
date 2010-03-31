import time
import sys

from pybloomfilter import BloomFilter
#from pybloom import BloomFilter

#bf = BloomFilter(72000, 0.1, './dict.bloom')
#bf = BloomFilter(72000, 0.1)

start = time.time()
for word in sys.stdin:
    word = word.strip()
    if word:
        bf.add(word)
print "%0.5f" % (time.time() - start)

