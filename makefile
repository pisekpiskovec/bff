CppC=g++

SRC_DIR=src
BUILD_DIR=build

INSTALL_LOCAL_DIR=$(HOME)/.local/bin
INSTALL_GLOBAL_DIR=/usr/local/bin
MANUAL_LOCAL_DIR=$(HOME)~/.local/share/man
MANUAL_GLOBAL_DIR=/usr/share/man/man1

build: always
	$(CppC) $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/bff
	cp makehelp $(BUILD_DIR)/makehelp

always:
	mkdir -p $(BUILD_DIR)

install: always build
	mkdir -p $(INSTALL_LOCAL_DIR)
	cp $(BUILD_DIR)/bff $(INSTALL_LOCAL_DIR)/bff
	mkdir -p $(MANUAL_LOCAL_DIR)
	cp makehelp $(MANUAL_LOCAL_DIR)/bff

ginstall: always build
	mkdir -p $(INSTALL_LOCAL_DIR)
	cp $(BUILD_DIR)/bff $(INSTALL_LOCAL_DIR)/bff
	mkdir -p $(MANUAL_GLOBAL_DIR)
	cp makehelp $(MANUAL_GLOBAL_DIR)/bff

help:
	@cat makehelp
