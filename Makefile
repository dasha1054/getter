# Makefile — 3D shooter (Quake 1 style), Linux build
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall
SRC       = src/main.cpp src/game.cpp src/map.cpp src/weapons.cpp src/renderer.cpp src/platform_x11.cpp
BIN       = quake3d
LIBS      = -lX11 -lm

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LIBS)

# headless check (без X11-окна, только логика + физика Quake)
test: $(SRC)
	./$(BIN) --benchmark || echo "run benchmark manually"

clean:
	rm -f $(BIN) *.o

.PHONY: all test clean
