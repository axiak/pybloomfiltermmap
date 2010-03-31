bloomfilter: mmapbitarray.* bloomfilter.*
	gcc -lm -O3 mmapbitarray.c bloomfilter.c -o bf

mbarray: mmapbitarray.*
	gcc -lm -O3 -DMBAQUERY mmapbitarray.c -o mbaquery
	gcc -lm -O3 -DMBACREATE mmapbitarray.c -o mbacreate
