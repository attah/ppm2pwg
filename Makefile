CXXFLAGS = -std=c++11 -O3 -pedantic -Wall -Wextra  -I. -Ibytestream \
-I/usr/include/glib-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/cairo/

VPATH = bytestream

all: ppm2pwg pwg2ppm

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

ppm2pwg: bytestream.o ppm2pwg.o ppm2pwg_main.o
	$(CXX) $^ -o $@

pwg2ppm: bytestream.o pwg2ppm.o
	$(CXX) $^ -o $@

pdf2printable: pdf2printable.o
	$(CXX) $^ -lpoppler -lcairo -lglib-2.0 -lpoppler-glib -lpoppler-cpp -o $@

clean:
	rm -f *.o ppm2pwg pwg2ppm
