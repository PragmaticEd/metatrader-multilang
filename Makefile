CXX=/usr/bin/i686-w64-mingw32-g++
CXXFLAGS=-s -shared -Wl,--kill-at -std=c++11 -m32 -static-libstdc++ -static-libgcc

all: server.dll

%.dll: %.cpp
	$(CXX) $^ -o $@ $(CXXFLAGS) -lzmq -lmsgpack
