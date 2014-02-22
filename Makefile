all:
	python setup.py build_ext --inplace

install:
	python setup.py install


clean:
	rm -rf build/
	rm -rf dist/
	rm -fv *so
