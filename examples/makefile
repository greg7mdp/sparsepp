CXXFLAGS     = -O2 -std=c++11 -I..
CXXFLAGS    += -Wall -pedantic -Wextra
SPP_DEPS_1   =  spp.h spp_utils.h spp_dlalloc.h spp_traits.h spp_config.h
SPP_DEPS     = $(addprefix ../sparsepp/,$(SPP_DEPS_1))
TARGETS      = insert_rvalue movable emplace piecewise hash_std serialize_file serialize_stream serialize_large

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

all: $(TARGETS)

clean:
	rm -f $(TARGETS) ages.dmp data.dat vsprojects/x64/* vsprojects/x86/*

%: %.cc $(SPP_DEPS) makefile
	$(CXX) $(CXXFLAGS) -DNDEBUG $< -o $@ $(LDFLAGS)

