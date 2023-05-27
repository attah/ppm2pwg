LDFLAGS = -ldl -Wl,--export-dynamic
CXXFLAGS = -std=c++11 -g -pedantic -Wall -Werror -Wextra -I ../bytestream
TEST_INCLUDES = -I ../lib -I ../bytestream/minitest -I ../bytestream/minitest/tests

VPATH = ../bytestream ../lib

all: test

test.o: test.cpp
	$(CXX) -c $(TEST_INCLUDES) $(CXXFLAGS) $^

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

test: bytestream.o printparameters.o pwg2ppm.o test.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f test *.o
