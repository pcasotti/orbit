CC = g++
CFLAGS = -std=c++17 -O2 -I./include
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXi

MAIN = orbit
SRC = src
OBJ = obj
SRCS = $(wildcard $(SRC)/*.cpp)
HDRS = $(wildcard $(SRC)/*.hpp)
OBJS = $(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(SRCS))

all: $(MAIN)

debug: $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $(MAIN) $(SRCS) $(LDFLAGS) -ggdb

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(OBJ)/%.o: $(SRC)/%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ)/%.o: $(SRC)/%.cpp $(SRC)/%.hpp
	$(CC) $(CFLAGS) -c $< -o $@

test: $(MAIN)
	./$(MAIN)

clean:
	rm -f $(MAIN)
