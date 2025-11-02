CXX      = g++
CXX_FILE = $(wildcard *.cpp)
TARGET   = $(patsubst %.cpp,%,$(CXX_FILE))
CXXFLAGS = -g -std=c++11 -Wall -Werror -pedantic-errors -fmessage-length=0

all:
	$(CXX) $(CXXFLAGS) dictionary-cpu.cpp -o dictionary
clean:
	rm -f $(TARGET) $(TARGET).exe
