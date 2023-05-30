LDFLAGS = -ldl -Wl,--export-dynamic -lcurl -ljpeg $(shell pkg-config --libs poppler-glib)
CXXFLAGS = -std=c++17 -g -pedantic -Wall -Werror -Wextra -I ../lib -I ../bytestream \
$(shell pkg-config --cflags poppler-glib)
TEST_INCLUDES = -I ../lib -I ../bytestream/minitest -I ../bytestream/minitest/tests

VPATH = ../bytestream ../lib

all: test

test.o: test.cpp
	$(CXX) -c $(TEST_INCLUDES) $(CXXFLAGS) $^

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $<

test: bytestream.o ippprintjob.o curlrequester.o printparameters.o ppm2pwg.o pwg2ppm.o pdf2printable.o baselinify.o ippmsg.o ippattr.o test.o
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -f test *.o
