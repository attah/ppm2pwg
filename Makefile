CXXFLAGS = -std=c++11 -O3 -pedantic -Wall -Wextra  -I. -Ibytestream \
-I/usr/include/glib-2.0/ -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/cairo/

VPATH = bytestream

all: ppm2pwg pwg2ppm pdf2printable


pdf2printable_mad.o: pdf2printable.cpp
	$(CXX) -c -DMADNESS=1 $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

ppm2pwg: bytestream.o ppm2pwg.o ppm2pwg_main.o
	$(CXX) $^ -o $@

pwg2ppm: bytestream.o pwg2ppm.o
	$(CXX) $^ -o $@

pdf2printable: bytestream.o ppm2pwg.o pdf2printable.o pdf2printable_main.o
	$(CXX) $^ -lcairo -lglib-2.0 -lgobject-2.0 -lpoppler-glib -o $@

pdf2printable_mad: bytestream.o ppm2pwg.o pdf2printable_mad.o pdf2printable_main.o
	$(CXX) $^ -lglib-2.0 -lgobject-2.0 -ldl -o $@

clean:
	rm -f *.o ppm2pwg pwg2ppm pdf2printable pdf2printable_mad
