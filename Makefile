all:
	rm -fv src/pybloomfilter.c
	python setup.py build_ext --inplace


clean:
	rm -fv src/pybloomfilter.c
	rm -rf build/
	rm -fv *so
