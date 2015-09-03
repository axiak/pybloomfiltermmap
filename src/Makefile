CC ?= gcc

bloomfilter: mmapbitarray.* bloomfilter.*
	$(CC) $(CFLAGS) $(CPPFLAGS) -O3 mmapbitarray.c md5.c MurmurHash3.cpp bloomfilter.c -o bf -lm $(LDFLAGS)

mbarray: mmapbitarray.*
	$(CC) $(CFLAGS) $(CPPFLAGS) -O3 -DMBAQUERY mmapbitarray.c -o mbaquery -lm $(LDFLAGS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -O3 -DMBACREATE mmapbitarray.c -o mbacreate -lm $(LDFLAGS)
