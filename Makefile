CXXFLAGS = -std=c++11 -O3 -pedantic -Wall -Wextra -Werror -I. -Ibytestream \
$(shell pkg-config --cflags glib-2.0) $(shell pkg-config --cflags cairo)

VPATH = bytestream

all: ppm2pwg pwg2ppm pdf2printable


pdf2printable_mad.o: pdf2printable.cpp
	$(CXX) -c -DMADNESS=1 $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

ppm2pwg: bytestream.o ppm2pwg.o ppm2pwg_main.o
	$(CXX) $^ -o $@

pwg2ppm: bytestream.o pwg2ppm.o pwg2ppm_main.o
	$(CXX) $^ -o $@

pdf2printable: bytestream.o ppm2pwg.o pdf2printable.o pdf2printable_main.o
	$(CXX) $^ $(shell pkg-config --libs poppler-glib) -o $@

pdf2printable_mad: bytestream.o ppm2pwg.o pdf2printable_mad.o pdf2printable_main.o
	$(CXX) $^ $(shell pkg-config --libs gobject-2.0) -ldl -o $@

hexdump: bytestream.o hexdump.o
	$(CXX) $^ -o $@

clean:
	rm -f *.o ppm2pwg pwg2ppm pdf2printable pdf2printable_mad

analyze:
	clang++ --analyze $(CXXFLAGS) *.cpp
