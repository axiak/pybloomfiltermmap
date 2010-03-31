import sys

reference = set()
got = set()
for word in open('./filtered'):
    word = word.strip()
    reference.add(word)

badnum = 0
terrible = 0

for word in sys.stdin:
    word = word.strip()
    if word not in reference:
        badnum += 1
    got.add(word)


print "REALLY BAD: %s" % (reference - got)

print "total: %d, rate: %0.5f" % (badnum, badnum / float(len(reference)))
