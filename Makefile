# Compiler settings
CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -O2 -Iinclude
DEBUGFLAGS = -std=c++14 -Wall -Wextra -g -Iinclude

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = .

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

# Target executable
TARGET = $(BIN_DIR)/dbsys

# Default target
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link object files
$(TARGET): $(BUILD_DIR) $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Debug build
debug: CXXFLAGS = $(DEBUGFLAGS)
debug: clean $(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "Clean complete"

# Clean everything including data and output
distclean: clean
	rm -rf data/*.dat output/*
	@echo "Deep clean complete"

# Run tests with different buffer sizes
test: $(TARGET)
	@echo "Running performance tests..."
	@echo ""
	@echo "Test 1: Buffer size = 5 blocks"
	./$(TARGET) --join --outer-table data/part.dat --inner-table data/partsupp.dat \
		--outer-type PART --inner-type PARTSUPP --output output/result_buf5.dat \
		--buffer-size 5
	@echo ""
	@echo "Test 2: Buffer size = 10 blocks"
	./$(TARGET) --join --outer-table data/part.dat --inner-table data/partsupp.dat \
		--outer-type PART --inner-type PARTSUPP --output output/result_buf10.dat \
		--buffer-size 10
	@echo ""
	@echo "Test 3: Buffer size = 20 blocks"
	./$(TARGET) --join --outer-table data/part.dat --inner-table data/partsupp.dat \
		--outer-type PART --inner-type PARTSUPP --output output/result_buf20.dat \
		--buffer-size 20
	@echo ""
	@echo "Test 4: Buffer size = 50 blocks"
	./$(TARGET) --join --outer-table data/part.dat --inner-table data/partsupp.dat \
		--outer-type PART --inner-type PARTSUPP --output output/result_buf50.dat \
		--buffer-size 50

# Help
help:
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  debug     - Build with debug symbols"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo "  test      - Run performance tests with different buffer sizes"
	@echo "  help      - Show this help message"

.PHONY: all debug clean distclean test help
