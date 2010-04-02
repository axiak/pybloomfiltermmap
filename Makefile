all:
	rm -fv src/pybloomfilter.c
	python setup.py build_ext --inplace

install:
	python setup.py install


clean:
	#rm -fv src/pybloomfilter.c
	rm -rf build/
	rm -rf dist/
	rm -fv *so
