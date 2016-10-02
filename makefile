all: spp_test 

clean: 
	/bin/rm spp_test

test:
	./spp_test

spp_test: spp_test.cc sparsepp.h makefile
	$(CXX) -O2 -std=c++0x -Wall -pedantic -Wextra -D_XOPEN_SOURCE=700 -D_CRT_SECURE_NO_WARNINGS spp_test.cc -o spp_test

