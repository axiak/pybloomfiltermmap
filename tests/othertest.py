#! /usr/bin/env python

import pybloomfilter

def main():
    bf = pybloomfilter.BloomFilter(100, 0.01, '/tmp/x.bf')
    bf.add(1234)
    print 1234 in bf

if __name__ == '__main__':
    main()
