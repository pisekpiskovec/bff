CC=gcc

SRC_DIR=src
BUILD_DIR=build

.PHONY: executable

executable: $(BUILD_DIR)/bff

$(BUILD_DIR)/bff: always
	$(CC) $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/bff

always:
	mkdir -p $(BUILD_DIR)
