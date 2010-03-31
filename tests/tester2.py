import time
import sys

from pybloomfilter import BloomFilter
from pybloom import BloomFilter

#bf = BloomFilter(120000, 0.1, './dict.bloom')

bf = BloomFilter(72000, 0.1)

for word in open('./filtered'):
    word = word.strip()
    if word:
        bf.add(word)

start = time.time()
for word in sys.stdin:
    word = word.strip()
    if word in bf:
        print word

sys.stderr.write("%0.5f\n" % (time.time() - start))

