.PHONY: ppm2pwg

ppm2pwg:
	g++ -std=c++11 -O3 -Wall -Wextra  -I. -Ibytestream ppm2pwg.cpp bytestream/bytestream.cpp -o ppm2pwg
