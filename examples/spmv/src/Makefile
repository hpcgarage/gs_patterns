###############################################################################
# Makefile for building spmv from spmv.c using g++ (and mmio.c with gcc)
# Location: examples/spmv/src
# Usage:
#   make         # build spmv
#   make clean   # remove build artifacts
###############################################################################

# Compilers
CXX ?= g++
CC  ?= gcc

# Flags
CXXFLAGS ?= -O3 -Wall -Wextra
CFLAGS   ?= -O3 -Wall -Wextra
LDFLAGS  ?=
LDLIBS   ?= -lm

# Sources and target
TARGET   := spmv
SRC_CXX  := spmv.c     # will be compiled with g++
SRC_C    := mmio.c     # compiled as C
OBJS     := $(SRC_CXX:.c=.o) $(SRC_C:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

# Compile all .c sources as C++ with g++ to keep symbols consistent
%.o: %.c
	$(CXX) $(CXXFLAGS) -x c++ -c $< -o $@

clean:
	rm -f $(TARGET) *.o


