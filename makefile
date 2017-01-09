all: spp_test 

clean: 
	/bin/rm spp_test

test:
	./spp_test

spp_test: spp_test.cc sparsepp.h makefile
	$(CXX) -O2 -std=c++0x -Wall -pedantic -Wextra -D_XOPEN_SOURCE=700 -D_CRT_SECURE_NO_WARNINGS spp_test.cc -o spp_test

spp_alloc_test: spp_alloc_test.cc spp_alloc.h spp_bitset.h sparsepp.h makefile
	$(CXX) -O2 -DNDEBUG  -std=c++11  spp_alloc_test.cc -o spp_alloc_test

perftest1: perftest1.cc sparsepp.h makefile
	$(CXX) -O2 -DNDEBUG  -std=c++11 perftest1.cc -o perftest1

