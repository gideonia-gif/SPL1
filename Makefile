# SPL Assignment 1 - Makefile
# Builds bin/server from every .cpp file in src/.

CXX      = g++
CXXFLAGS = -g -Wall -Weffc++ -std=c++11

# Every src/X.cpp becomes bin/X.o (new .cpp files are picked up automatically)
SRCS    = $(wildcard src/*.cpp)
OBJS    = $(patsubst src/%.cpp,bin/%.o,$(SRCS))
HEADERS = $(wildcard include/*.h)

all: bin/server

# Link step: combine all object files into the executable
bin/server: $(OBJS)
	$(CXX) -o $@ $^

# Compile step: one object file per source file.
# Depends on the headers too, so changing a .h rebuilds what uses it.
bin/%.o: src/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c -Iinclude $< -o $@

# Remove compiled files but keep the bin/ directory
clean:
	rm -f bin/*.o bin/server

.PHONY: all clean
