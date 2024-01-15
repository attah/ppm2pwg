CXXFLAGS = -std=c++17 -O3 -pedantic -Wall -Wextra -Werror -Ilib -Ibytestream \
$(shell pkg-config --cflags poppler-glib)

VPATH = bytestream lib utils

all: ppm2pwg pwg2ppm pdf2printable hexdump baselinify ippposter


pdf2printable_mad.o: pdf2printable.cpp
	$(CXX) -c -DMADNESS=1 $(CXXFLAGS) $^ -o $@

baselinify_mad.o: baselinify.cpp
	$(CXX) -c -DMADNESS=1 $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

ppm2pwg: bytestream.o printparameters.o ppm2pwg.o ppm2pwg_main.o
	$(CXX) $^ -o $@

pwg2ppm: bytestream.o pwg2ppm.o pwg2ppm_main.o
	$(CXX) $^ -o $@

pdf2printable: bytestream.o printparameters.o ppm2pwg.o pdf2printable.o pdf2printable_main.o
	$(CXX) $^ $(shell pkg-config --libs poppler-glib) -o $@

pdf2printable_mad: bytestream.o printparameters.o ppm2pwg.o pdf2printable_mad.o pdf2printable_main.o
	$(CXX) $^ $(shell pkg-config --libs gobject-2.0) -ldl -o $@

hexdump: bytestream.o hexdump.o
	$(CXX) $^ -o $@

baselinify: bytestream.o baselinify.o baselinify_main.o
	$(CXX) $^ -ljpeg -o $@

baselinify_mad: bytestream.o baselinify_mad.o baselinify_main.o
	$(CXX) $^ -ldl -o $@

ippdecode: bytestream.o ippmsg.o ippattr.o ippdecode.o
	$(CXX) $^ -o $@

ippposter: ippmsg.o ippattr.o ippprintjob.o printparameters.o ippposter.o curlrequester.o minimime.o pdf2printable.o ppm2pwg.o baselinify.o bytestream.o
	$(CXX) $^ $(shell pkg-config --libs poppler-glib) -ljpeg -lcurl -o $@

minimime: minimime_main.o minimime.o bytestream.o
	$(CXX) $^ -o $@

clean:
	rm -f *.o ppm2pwg pwg2ppm pdf2printable pdf2printable_mad hexdump baselinify baselinify_mad ippposter minimime

analyze:
	clang++ --analyze $(CXXFLAGS) lib/*.cpp utils/*.cpp

fuzz:
	clang++ -g -O1 -fsanitize=fuzzer $(CXXFLAGS) -O1 -DFUZZ lib/ippmsg.cpp lib/ippattr.cpp bytestream/bytestream.cpp -o $@
