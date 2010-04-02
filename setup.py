from distutils.core import setup
from distutils.extension import Extension

ext_files = ["src/mmapbitarray.c",
             "src/bloomfilter.c",
             ]

kwargs = {}

try:
    from Cython.Distutils import build_ext
    print "Building from Cython"
    ext_files.append("src/pybloomfilter.pyx")
    kwargs['cmdclass'] = {'build_ext': build_ext}
except ImportError:
    ext_files.append("src/pybloomfilter.c")
    print "Building from C"

ext_modules = [Extension("pybloomfilter",
                         ext_files)]

setup(
  name = 'pybloomfiltermmap',
  version = "0.1.10",
  author = "Michael Axiak",
  author_email = "mike@axiak.net",
  url = "http://code.google.com/p/python-bloom-filter/",
  description = "A Bloom filter for Python built on mmap",
  license = "MIT License",
  ext_modules = ext_modules,
  **kwargs
)
 
