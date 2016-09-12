all: test

test:
	c++ -o /tmp/test test.cpp -DHAVE_ZLIB -DKENLM_MAX_ORDER=6 -I./kenlm -L./kenlm/build/lib -lkenlm -lkenlm_util -lz -lbz2 -lglog -lgflags -std=c++11 -O3 -g

