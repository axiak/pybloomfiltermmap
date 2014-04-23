all:
	python setup.py build_ext --inplace

install:
	@# Support Debian package building with fall-back default
	python setup.py install --root $${DESTDIR:-/}

test:
	python setup.py test


clean:
	rm -rf build/
	rm -rf dist/
	rm -fv *so
