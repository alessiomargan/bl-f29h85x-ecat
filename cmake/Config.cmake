# CMake configuration file for common settings
# This file can be sourced to set up the build environment

# Compiler paths (update these to match your installation)
set(TI_CLANG_ROOT "/opt/ti/clang" CACHE PATH "TI Clang compiler root")
set(MCU_SDK_ROOT "/opt/ti/mcu_sdk_f29h85x" CACHE PATH "MCU SDK root")

# Default build type
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")

# Feature flags
set(USE_SYSCFG "ON" CACHE BOOL "Use SysConfig generated files")
set(CREATE_BIN "ON" CACHE BOOL "Create binary output")
set(CREATE_HEX "OFF" CACHE BOOL "Create HEX output")

# Optimization level (can be overridden)
# set(CMAKE_C_FLAGS_RELEASE "-O2" CACHE STRING "Release optimization")
