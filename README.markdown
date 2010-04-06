# pybloomfiltermmap

The goal of `pybloomfiltermmap` is simple: to provide a fast, simple, scalable,
correct library for Bloom Filters in Python.

## Docs

You should probably read the docs online at http://mike.axiak.net/python-bloom-filter/docs/html/

## Overview

After you install, the interface to use is a cross between a file
interface and a ste interface. As an example:

    >>> fruit = pybloomfilter.BloomFilter(100000, 0.1, '/tmp/words.bloom')
    >>> fruit.extend(('apple', 'pear', 'orange', 'apple'))
    >>> len(bf)
    3
    >>> 'mike' in fruit
    False

## Install

You may or may not want to use Cython. If you have it installed, the
setup file will build the C file from the pyx file. Otherwise, it will
skip that step automatically and build from the packaged C file.

To install:

   $ sudo python setup.py install

and you should be set.

## License

See the LICENSE file. I am licensing it under the MIT License.

