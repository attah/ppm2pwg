.PHONY: all ppm2pwg pwg2ppm

all: ppm2pwg pwg2ppm

ppm2pwg:
	g++ -std=c++11 -O3 -pedantic -Wall -Wextra  -I. -Ibytestream ppm2pwg.cpp bytestream/bytestream.cpp -o ppm2pwg

pwg2ppm:
	g++ -std=c++11 -O3 -pedantic -Wall -Wextra  -I. -Ibytestream pwg2ppm.cpp bytestream/bytestream.cpp -o pwg2ppm
