all: spp_test 

clean: 
	/bin/rm spp_test

test:
	./spp_test

spp_test: spp_test.cc sparsepp.h
	$(CXX) -O2 -std=c++11 -D_CRT_SECURE_NO_WARNINGS spp_test.cc -o spp_test

time_hash_map: time_hash_map.cc sparsepp.h
	$(CXX) -O2 -std=c++11 -mpopcnt -D_CRT_SECURE_NO_WARNINGS -Iwindows time_hash_map.cc -o time_hash_map

