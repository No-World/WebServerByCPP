CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -pthread
LDFLAGS = -pthread

all: myhttp

myhttp: httpd.cpp
	$(CXX) $(CXXFLAGS) -o myhttp httpd.cpp $(LDFLAGS)

clean:
	rm -f myhttp