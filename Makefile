CC=g++
CFLAGS=-g -Wall -std=c++17 -O2

TARGET=blockchain_simulator
BUILD=build
SRC=src

SOURCES := $(wildcard $(SRC)/*.cpp)
HEADERS := $(wildcard $(SRC)/*.h)
OBJECTS := $(patsubst $(SRC)/%.cpp, $(BUILD)/%.o, $(SOURCES))

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJECTS): | $(BUILD)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: $(SRC)/%.cpp
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@

$(SRC)/%.cpp: $(HEADERS)

clean:
	rm -rf $(TARGET) $(BUILD)
