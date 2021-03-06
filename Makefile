CXXFLAGS = -std=c++11 -O3 -pedantic -Wall -Wextra  -I. -Ibytestream
VPATH = bytestream

all: ppm2pwg pwg2ppm

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

ppm2pwg: bytestream.o ppm2pwg.o
	$(CXX) $^ -o $@

pwg2ppm: bytestream.o pwg2ppm.o
	$(CXX) $^ -o $@

clean:
	rm -f *.o ppm2pwg pwg2ppm
