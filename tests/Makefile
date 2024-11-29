LDFLAGS = -ldl -Wl,--export-dynamic -lcurl -ljpeg $(shell pkg-config --libs poppler-glib) -lz
CXXFLAGS = -std=c++17 -g -pedantic -Wall -Werror -Wextra -I ../lib -I ../json11 -I ../bytestream \
$(shell pkg-config --cflags poppler-glib)
TEST_INCLUDES = -I ../lib -I ../bytestream/minitest

VPATH = ../bytestream ../lib ../json11

all: test

test.o: test.cpp
	$(CXX) -c $(TEST_INCLUDES) $(CXXFLAGS) $^

# Silly clang
json11.o: json11.cpp
	$(CXX) -c $(CXXFLAGS) -Wno-unqualified-std-cast-call $<

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

test: bytestream.o ippprinter.o ippprintjob.o curlrequester.o printparameters.o ppm2pwg.o pwg2ppm.o pdf2printable.o baselinify.o ippmsg.o ippattr.o json11.o minimime.o ippdiscovery.o test.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f test *.o
