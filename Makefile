CXXFLAGS = -std=c++17 -O3 -pedantic -Wall -Wextra -Werror -Ilib -Ibytestream -Ijson11 \
$(shell pkg-config --cflags poppler-glib) $(EXTRA_CXXFLAGS)

LDFLAGS = $(EXTRA_LDFLAGS)

CLANGXX ?= clang++
CLANG_TIDY ?= clang-tidy

# json11 has an explicit using declaration, yet clang throws this warning
SILLY_CLANG_FLAGS = -Wno-unqualified-std-cast-call

VPATH = bytestream lib utils json11

OFFICIAL = ppm2pwg pwg2ppm pdf2printable baselinify ippclient ippdiscover
EXTRAS = hexdump ippdecode bsplit minimime

all: $(OFFICIAL) $(EXTRAS)

pdf2printable_mad.o: pdf2printable.cpp
	$(CXX) -MMD -c -DMADNESS=1 $(CXXFLAGS) $^ -o $@

baselinify_mad.o: baselinify.cpp
	$(CXX) -MMD -c -DMADNESS=1 $(CXXFLAGS) $^ -o $@

json11.o: json11.cpp
	$(CXX) -MMD -c $(CXXFLAGS) $(SILLY_CLANG_FLAGS) $<

%.o: %.cpp
	$(CXX) -MMD -c $(CXXFLAGS) $<

ppm2pwg: bytestream.o printparameters.o ppm2pwg.o ppm2pwg_main.o
	$(CXX) $^ $(LDFLAGS) -o $@

pwg2ppm: bytestream.o pwg2ppm.o pwg2ppm_main.o
	$(CXX) $^ $(LDFLAGS) -o $@

pdf2printable: bytestream.o printparameters.o ppm2pwg.o pdf2printable.o pdf2printable_main.o
	$(CXX) $^ $(shell pkg-config --libs poppler-glib) $(LDFLAGS) -o $@

pdf2printable_mad: bytestream.o printparameters.o ppm2pwg.o pdf2printable_mad.o pdf2printable_main.o
	$(CXX) $^ $(shell pkg-config --libs gobject-2.0) -ldl  $(LDFLAGS) -o $@

hexdump: bytestream.o hexdump.o
	$(CXX) $^ $(LDFLAGS) -o $@

baselinify: bytestream.o baselinify.o baselinify_main.o
	$(CXX) $^ $(shell pkg-config --libs libjpeg) $(LDFLAGS) -o $@

baselinify_mad: bytestream.o baselinify_mad.o baselinify_main.o
	$(CXX) $^ -ldl -o $@

ippdecode: bytestream.o ippmsg.o ippattr.o json11.o ippdecode.o
	$(CXX) $^ $(LDFLAGS) -o $@

bsplit: bytestream.o bsplit.o
	$(CXX) $^ $(LDFLAGS) -o $@

ippclient: ippmsg.o ippattr.o ippprinter.o ippprintjob.o printparameters.o ippclient.o json11.o curlrequester.o minimime.o pdf2printable.o ppm2pwg.o baselinify.o bytestream.o
	$(CXX) $^ $(shell pkg-config --libs poppler-glib) $(shell pkg-config --libs libjpeg) -lcurl -lz -lpthread $(LDFLAGS) -o $@

minimime: minimime_main.o minimime.o bytestream.o
	$(CXX) $^ $(LDFLAGS) -o $@

ippdiscover: ippdiscover.o ippdiscovery.o bytestream.o
	$(CXX) $^ $(LDFLAGS) -o $@

clean:
	rm -f *.o *.d $(OFFICIAL) $(EXTRAS)

%.tidy: %.cpp
	$(CLANG_TIDY) $< -- $(CXXFLAGS)

TIDY_SOURCES = $(wildcard lib/*.cpp) $(wildcard utils/*.cpp)

tidy: $(subst .cpp,.tidy, $(TIDY_SOURCES))

fuzz:
	$(CLANGXX) -g -fsanitize=fuzzer $(CXXFLAGS) $(SILLY_CLANG_FLAGS) -O0 -DFUZZ lib/ippmsg.cpp lib/ippattr.cpp bytestream/bytestream.cpp json11/json11.cpp -o $@

install: $(OFFICIAL)
	cp $^ /usr/bin/

-include *.d
