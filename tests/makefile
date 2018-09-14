CXXSTD      ?= c++11
CXXFLAGS     = -O2 -std=$(CXXSTD) -I..
CXXFLAGS    += -Wall -pedantic -Wextra
SPP_DEPS_1   =  spp.h spp_utils.h spp_dlalloc.h spp_traits.h spp_config.h
SPP_DEPS     = $(addprefix ../sparsepp/,$(SPP_DEPS_1))
TARGETS      = spp_test spp_alloc_test spp_bitset_test perftest1 bench


ifeq ($(OS),Windows_NT)
    LDFLAGS  = -lpsapi
else
    OS = $(shell uname -s)
    ifeq ($(OS),Linux)
        CXXFLAGS        += -D_XOPEN_SOURCE=700
    endif
    ifeq ($(OS),FreeBSD)
	LDFLAGS		= -lkvm
    endif
endif

def: spp_test spp_relative_include_test

all: $(TARGETS)

clean:
	rm -rf $(TARGETS) vsprojects/x64/* vsprojects/x86/*

test:
	./spp_test

spp_test: spp_test.cc $(SPP_DEPS) makefile
	$(CXX) $(CXXFLAGS) -D_CRT_SECURE_NO_WARNINGS spp_test.cc -o spp_test

# Test that it's possible to use spp with relative includes only - without adding -I to the compiler
spp_relative_include_test: spp_relative_include_test.cc $(SPP_DEPS) makefile
	$(CXX) $(filter-out -I%,$(CXXFLAGS)) -D_CRT_SECURE_NO_WARNINGS spp_relative_include_test.cc -o spp_relative_include_test

%: %.cc $(SPP_DEPS) makefile
	$(CXX) $(CXXFLAGS) -DNDEBUG $< -o $@ $(LDFLAGS)

