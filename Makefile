# Top-level helper Makefile for CMake-based TI C29 builds.

CMAKE ?= cmake
BUILD_DIR ?= build_vscode
BUILD_TYPE ?= Debug
TARGET_CONFIG ?= FLASH
CREATE_BIN ?= ON
CREATE_HEX ?= OFF

# Override these from your shell or on command line if needed.
TI_CLANG_ROOT ?= /home/amargan/ti/ccs2100/ccs/tools/compiler/ti-cgt-c29_2.2.0.LTS
MCU_SDK_ROOT ?= /home/amargan/ti/f29h85x-sdk_1_02_01_00
SYSCONFIG_ROOT ?= /home/amargan/ti/sysconfig_1.27.1
DSLITE ?= /home/amargan/ti/uniflash_9.6.0/dslite.sh
TARGET_CONFIG_FILE ?= $(CURDIR)/targetConfigs/F29H850TU9.ccxml

BIN_FILE := $(BUILD_DIR)/bl-f29h85x-ecat.bin
OUT_FILE := $(BUILD_DIR)/bl-f29h85x-ecat.out

.PHONY: help configure build clean rebuild flash flash-bin bin dslite-flash flash-probe

help:
	@echo "Usage: make <target> [VAR=value]"
	@echo ""
	@echo "Targets:"
	@echo "  configure   Configure CMake in $(BUILD_DIR)"
	@echo "  build       Configure and build"
	@echo "  clean       Remove $(BUILD_DIR)"
	@echo "  rebuild     Clean then build"
	@echo "  flash-bin   Build FLASH configuration and generate .bin"
	@echo "  dslite-flash Build FLASH .out and program it via dslite.sh"
	@echo "  flash-probe Alias of dslite-flash"
	@echo "  flash       Alias of flash-bin"
	@echo "  bin         Alias of flash-bin"
	@echo ""
	@echo "Common overrides:"
	@echo "  BUILD_TYPE=Debug|Release"
	@echo "  BUILD_DIR=build_vscode"
	@echo "  TI_CLANG_ROOT=/path/to/ti-cgt-c29"
	@echo "  MCU_SDK_ROOT=/path/to/f29h85x-sdk"
	@echo "  SYSCONFIG_ROOT=/path/to/sysconfig"
	@echo "  DSLITE=/path/to/dslite.sh"
	@echo "  TARGET_CONFIG_FILE=$(CURDIR)/targetConfigs/F29H850TU9.ccxml"

configure:
	@$(CMAKE) -B $(BUILD_DIR) -S . \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DTARGET_CONFIG=$(TARGET_CONFIG) \
		-DCREATE_BIN=$(CREATE_BIN) \
		-DCREATE_HEX=$(CREATE_HEX) \
		-DTI_CLANG_ROOT=$(TI_CLANG_ROOT) \
		-DMCU_SDK_ROOT=$(MCU_SDK_ROOT) \
		-DSYSCONFIG_ROOT=$(SYSCONFIG_ROOT)

build: configure
	@$(CMAKE) --build $(BUILD_DIR) -j

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Removed $(BUILD_DIR)"

rebuild: clean build

flash-bin:
	@$(MAKE) build TARGET_CONFIG=FLASH CREATE_BIN=ON
	@test -f $(BIN_FILE)
	@echo "FLASH BIN ready: $(BIN_FILE)"

dslite-flash:
	@$(MAKE) build TARGET_CONFIG=FLASH CREATE_BIN=ON
	@test -f $(OUT_FILE)
	@$(DSLITE) --config=$(TARGET_CONFIG_FILE) --flash $(OUT_FILE)
	@echo "Programmed via dslite: $(OUT_FILE)"

flash-probe: dslite-flash

flash: flash-bin

bin: flash-bin
