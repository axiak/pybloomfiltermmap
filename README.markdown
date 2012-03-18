# pybloomfiltermmap [![Build Status](https://secure.travis-ci.org/axiak/pybloomfiltermmap.png?branch=master)](http://travis-ci.org/axiak/pybloomfiltermmap)

The goal of `pybloomfiltermmap` is simple: to provide a fast, simple, scalable,
correct library for Bloom Filters in Python.

## Docs

See <http://axiak.github.com/pybloomfiltermmap/>.

## Overview

After you install, the interface to use is a cross between a file
interface and a ste interface. As an example:

    >>> fruit = pybloomfilter.BloomFilter(100000, 0.1, '/tmp/words.bloom')
    >>> fruit.extend(('apple', 'pear', 'orange', 'apple'))
    >>> len(fruit)
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

See the LICENSE file. It's under the MIT License.

