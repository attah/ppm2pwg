LDFLAGS = -ldl -Wl,--export-dynamic
CXXFLAGS = -std=c++11 -g -pedantic -Wall -Werror -Wextra

.PHONY: test
# test.cpp listed explicitly since changing test.h would go unnoticed otherwise
test:
	$(CXX) $(CXXFLAGS) \
	-I .. -I ../bytestream -I ../bytestream/minitest -I ../bytestream/minitest/tests \
	../bytestream/bytestream.cpp test.cpp $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -f test
