import os
import sys

here = os.path.dirname(__file__)

ext_files = ["src/mmapbitarray.c",
             "src/bloomfilter.c",
             "src/md5.c",
             "src/primetester.c",
             ]

kwargs = {}

try:
    import Cython
    sys.path.insert(0, os.path.join(here, 'fake_pyrex'))
except ImportError:
    pass


from setuptools import setup, Extension

try:
    from Cython.Distutils import build_ext
    print "info: Building from Cython"
    ext_files.append("src/pybloomfilter.pyx")
    kwargs['cmdclass'] = {'build_ext': build_ext}
    try:
        os.unlink(os.path.join(here, 'src', 'pybloomfilter.c'))
        os.unlink(os.path.join(here, 'pybloomfilter.so'))
    except:
        pass
except ImportError:
    if '--cython' in sys.argv:
        raise
    ext_files.append("src/pybloomfilter.c")
    print "info: Building from C"

if '--cython' in sys.argv:
    sys.argv.remove('--cython')

ext_modules = [Extension("pybloomfilter",
                         ext_files,
                         libraries=['crypto'])]

requirements = []

if sys.version_info[0] < 3 and sys.version_info[1] < 7:
    requirements.append('importlib')

setup(
  name = 'pybloomfiltermmap',
  version = "0.3.8",
  author = "Michael Axiak, Rob Stacey",
  author_email = "mike@axiak.net",
  url = "http://github.com/axiak/pybloomfiltermmap/",
  description = "A Bloom filter (bloomfilter) for Python built on mmap",
  license = "MIT License",
  test_suite = 'tests.test_all',
  install_requires=requirements,
  ext_modules = ext_modules,
  classifiers = [
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: C',
        'Programming Language :: Cython',
        'Programming Language :: Python',
        'Topic :: Software Development :: Libraries :: Python Modules',
        ],
  **kwargs
)

