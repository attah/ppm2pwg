LDFLAGS = -ldl -Wl,--export-dynamic
CXXFLAGS = -std=c++11 -g -pedantic -Wall -Werror -Wextra -I ../bytestream
TEST_INCLUDES = -I .. -I ../bytestream/minitest -I ../bytestream/minitest/tests

all: test

bytestream.o: ../bytestream/bytestream.cpp
	$(CXX) -c $(CXXFLAGS) $^

pwg2ppm.o: ../pwg2ppm.cpp
	$(CXX) -c $(CXXFLAGS) $^

test.o: test.cpp
	$(CXX) -c $(TEST_INCLUDES) $(CXXFLAGS) $^

test: bytestream.o pwg2ppm.o test.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f test *.o
