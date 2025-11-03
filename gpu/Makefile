# Simple Makefile for CUDA + MPI
TARGET = dictionary
SRC = dictionary.cu

# Compile flags
NVCC = nvcc
MPICXX = mpicxx
NVCC_FLAGS = -O3 -std=c++17 -allow-unsupported-compiler

# Default rule
$(TARGET): $(SRC)
	$(NVCC) $(NVCC_FLAGS) -ccbin $(MPICXX) -o $@ $<

clean:
	rm -f $(TARGET)

