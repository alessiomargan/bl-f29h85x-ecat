cmake_minimum_required(VERSION 3.20)

# Build for embedded C29 target, not host.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR c29x)

# Avoid link-time checks during compiler probing.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES TI_CLANG_ROOT)

# Resolve TI toolchain root from cache or environment.
if(NOT TI_CLANG_ROOT)
	if(DEFINED CMAKE_C_COMPILER AND NOT "${CMAKE_C_COMPILER}" STREQUAL "")
		get_filename_component(_compiler_bin_dir "${CMAKE_C_COMPILER}" DIRECTORY)
		get_filename_component(TI_CLANG_ROOT "${_compiler_bin_dir}/.." ABSOLUTE)
	endif()
endif()

if(NOT TI_CLANG_ROOT)
	if(DEFINED ENV{TI_CLANG_ROOT} AND NOT "$ENV{TI_CLANG_ROOT}" STREQUAL "")
		set(TI_CLANG_ROOT "$ENV{TI_CLANG_ROOT}")
	endif()
endif()

if(NOT TI_CLANG_ROOT)
	message(FATAL_ERROR "TI_CLANG_ROOT not set. Pass -DTI_CLANG_ROOT=/path/to/ti-cgt-c29 or export TI_CLANG_ROOT.")
endif()

set(CMAKE_C_COMPILER "${TI_CLANG_ROOT}/bin/c29clang" CACHE FILEPATH "C compiler" FORCE)
set(CMAKE_ASM_COMPILER "${TI_CLANG_ROOT}/bin/c29clang" CACHE FILEPATH "ASM compiler" FORCE)
set(CMAKE_AR "${TI_CLANG_ROOT}/bin/c29ar" CACHE FILEPATH "Archiver" FORCE)

if(NOT EXISTS "${CMAKE_C_COMPILER}")
	message(FATAL_ERROR "TI C29 compiler not found at ${CMAKE_C_COMPILER}")
endif()

# Match SDK makefiles baseline.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_INIT "-Qunused-arguments -std=c1x")
set(CMAKE_ASM_FLAGS_INIT "-Qunused-arguments")

message(STATUS "TI C29 toolchain configured")
message(STATUS "  TI_CLANG_ROOT: ${TI_CLANG_ROOT}")
message(STATUS "  C compiler: ${CMAKE_C_COMPILER}")
