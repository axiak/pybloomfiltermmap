from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext

ext_modules = [Extension("pybloomfilter",
                         ["src/pybloomfilter.pyx",
                          "src/primetester.c",
                          "src/bloomfilter.c",
                          "src/mmapbitarray.c",
                          ])]

setup(
  name = 'C Bloom Filter',
  cmdclass = {'build_ext': build_ext},
  ext_modules = ext_modules
)
