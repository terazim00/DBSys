# Compiler settings
CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -O2 -Iinclude
DEBUGFLAGS = -std=c++14 -Wall -Wextra -g -Iinclude

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = .
EXAMPLE_DIR = examples

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))

# Target executable
TARGET = $(BIN_DIR)/dbsys

# Example executables
EXAMPLE_SIMPLE = $(BIN_DIR)/simple_usage
EXAMPLE_FULL = $(BIN_DIR)/file_manager_example
EXAMPLE_JOIN = $(BIN_DIR)/join_demo
EXAMPLE_PERF = $(BIN_DIR)/performance_test

# Library objects (without main.cpp)
LIB_SOURCES = $(filter-out $(SRC_DIR)/main.cpp, $(SOURCES))
LIB_OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(LIB_SOURCES))

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

# Build example programs
examples: $(EXAMPLE_SIMPLE) $(EXAMPLE_FULL) $(EXAMPLE_JOIN) $(EXAMPLE_PERF)

$(EXAMPLE_SIMPLE): $(BUILD_DIR) $(LIB_OBJECTS) $(EXAMPLE_DIR)/simple_usage.cpp
	$(CXX) $(CXXFLAGS) $(LIB_OBJECTS) $(EXAMPLE_DIR)/simple_usage.cpp -o $(EXAMPLE_SIMPLE)
	@echo "Built: $(EXAMPLE_SIMPLE)"

$(EXAMPLE_FULL): $(BUILD_DIR) $(LIB_OBJECTS) $(EXAMPLE_DIR)/file_manager_example.cpp
	$(CXX) $(CXXFLAGS) $(LIB_OBJECTS) $(EXAMPLE_DIR)/file_manager_example.cpp -o $(EXAMPLE_FULL)
	@echo "Built: $(EXAMPLE_FULL)"

$(EXAMPLE_JOIN): $(BUILD_DIR) $(LIB_OBJECTS) $(EXAMPLE_DIR)/join_demo.cpp
	$(CXX) $(CXXFLAGS) $(LIB_OBJECTS) $(EXAMPLE_DIR)/join_demo.cpp -o $(EXAMPLE_JOIN)
	@echo "Built: $(EXAMPLE_JOIN)"

$(EXAMPLE_PERF): $(BUILD_DIR) $(LIB_OBJECTS) $(EXAMPLE_DIR)/performance_test.cpp
	$(CXX) $(CXXFLAGS) $(LIB_OBJECTS) $(EXAMPLE_DIR)/performance_test.cpp -o $(EXAMPLE_PERF)
	@echo "Built: $(EXAMPLE_PERF)"

# Clean examples
clean-examples:
	rm -f $(EXAMPLE_SIMPLE) $(EXAMPLE_FULL) $(EXAMPLE_JOIN) $(EXAMPLE_PERF)
	@echo "Example executables cleaned"

# Help
help:
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  debug     - Build with debug symbols"
	@echo "  examples  - Build example programs"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo "  test      - Run performance tests with different buffer sizes"
	@echo "  help      - Show this help message"

.PHONY: all debug clean distclean test help examples clean-examples
