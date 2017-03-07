# pybloomfiltermmap [![Build Status](https://secure.travis-ci.org/axiak/pybloomfiltermmap.png?branch=master)](http://travis-ci.org/axiak/pybloomfiltermmap)

The goal of `pybloomfiltermmap` is simple: to provide a fast, simple, scalable,
correct library for Bloom Filters in Python.

## Docs

See <http://axiak.github.com/pybloomfiltermmap/>.

## Overview

After you install, the interface to use is a cross between a file
interface and a [set](https://docs.python.org/2/library/sets.html) interface. As an example:

    >>> fruit = pybloomfilter.BloomFilter(100000, 0.1, '/tmp/words.bloom')
    >>> fruit.update(('apple', 'pear', 'orange', 'apple'))
    >>> len(fruit)
    3
    >>> 'mike' in fruit
    False
    >>> 'apple' in fruit
    True

## Install

If you have [Cython](http://cython.org/) installed it will generate
the C extension from the pyx file. Otherwise it will use the
pre-generated C code. Cython is only required for development.

To install (in the git checkout):
  
    $ pip install cython 
    $ pip install -e .

Or directly from pypi:

    $ pip install pybloomfiltermmap

and you should be set.

## Test

    $ python setup.py test

## License

See the LICENSE file. It's under the MIT License.
