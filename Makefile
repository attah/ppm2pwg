.PHONY: ppm2pwg

ppm2pwg:
	g++ -O3 -Wall -Wextra  -I. -Ibytestream ppm2pwg.cpp bytestream/bytestream.cpp -o ppm2pwg
