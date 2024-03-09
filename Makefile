CXXFLAGS = -std=c++17 -O3 -pedantic -Wall -Wextra -Werror -Ilib -Ibytestream -Ijson11 \
$(shell pkg-config --cflags poppler-glib)

VPATH = bytestream lib utils json11

all: ppm2pwg pwg2ppm pdf2printable baselinify ippclient hexdump ippdecode


pdf2printable_mad.o: pdf2printable.cpp
	$(CXX) -c -DMADNESS=1 $(CXXFLAGS) $^ -o $@

baselinify_mad.o: baselinify.cpp
	$(CXX) -c -DMADNESS=1 $(CXXFLAGS) $^ -o $@

# Silly clang
json11.o: json11.cpp
	$(CXX) -c $(CXXFLAGS) -Wno-unqualified-std-cast-call $<

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

ippdecode: bytestream.o ippmsg.o ippattr.o json11.o ippdecode.o
	$(CXX) $^ -o $@

ippclient: ippmsg.o ippattr.o ippprinter.o ippprintjob.o printparameters.o ippclient.o json11.o curlrequester.o minimime.o pdf2printable.o ppm2pwg.o baselinify.o bytestream.o
	$(CXX) $^ $(shell pkg-config --libs poppler-glib) -ljpeg -lcurl -lz -o $@

minimime: minimime_main.o minimime.o bytestream.o
	$(CXX) $^ -o $@

clean:
	rm -f *.o ppm2pwg pwg2ppm pdf2printable pdf2printable_mad hexdump baselinify baselinify_mad ippclient minimime

analyze:
	clang++ --analyze $(CXXFLAGS) lib/*.cpp utils/*.cpp

tidy:
	clang-tidy lib/*.cpp utils/*.cpp -- $(CXXFLAGS)

fuzz:
	clang++ -g -fsanitize=fuzzer $(CXXFLAGS) -O0 -DFUZZ lib/ippmsg.cpp lib/ippattr.cpp bytestream/bytestream.cpp json11/json11.cpp -o $@
