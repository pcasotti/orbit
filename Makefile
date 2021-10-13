CFLAGS = -std=c++17 -O2 -I./include
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXi

orbit: src/*.cpp src/*.hpp
	g++ $(CFLAGS) -o orbit src/*.cpp $(LDFLAGS)

.PHONY: test clean

test: orbit
	./orbit

clean:
	rm -f orbit
