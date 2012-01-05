import os
import glob
import unittest
import importlib
import functools
import tempfile

here = os.path.dirname(__file__)

def with_test_file(method):
    @functools.wraps(method)
    def _wrapped(*args, **kwargs):
        f = tempfile.NamedTemporaryFile(suffix='.bloom')
        kwargs['filename'] = f.name
        try:
            return method(*args, **kwargs)
        finally:
            f.close()
    return _wrapped

def test_all():
    suite = unittest.TestSuite()
    for fname in glob.glob(os.path.join(here, '*.py')):
        if '__init__' in fname:
            continue
        module = importlib.import_module('tests.' + os.path.basename(fname).split('.py')[0])
        if hasattr(module, 'suite'):
            suite.addTest(module.suite())
    return suite
