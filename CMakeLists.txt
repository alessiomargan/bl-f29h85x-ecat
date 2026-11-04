cmake_minimum_required(VERSION 3.20)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Treat this as a cross-compilation target to avoid host assumptions.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR c29x)

# ==============================================================================
# TI Clang Compiler Setup
# ==============================================================================

# Set TI Clang toolchain paths
if((NOT DEFINED TI_CLANG_ROOT OR "${TI_CLANG_ROOT}" STREQUAL "") AND DEFINED ENV{TI_CLANG_ROOT} AND NOT "$ENV{TI_CLANG_ROOT}" STREQUAL "")
    set(TI_CLANG_ROOT "$ENV{TI_CLANG_ROOT}" CACHE PATH "TI Clang compiler root directory" FORCE)
endif()

if((NOT DEFINED MCU_SDK_ROOT OR "${MCU_SDK_ROOT}" STREQUAL "") AND DEFINED ENV{MCU_SDK_ROOT} AND NOT "$ENV{MCU_SDK_ROOT}" STREQUAL "")
    set(MCU_SDK_ROOT "$ENV{MCU_SDK_ROOT}" CACHE PATH "MCU SDK root directory" FORCE)
endif()

if((NOT DEFINED SYSCONFIG_ROOT OR "${SYSCONFIG_ROOT}" STREQUAL "") AND DEFINED ENV{SYSCONFIG_ROOT} AND NOT "$ENV{SYSCONFIG_ROOT}" STREQUAL "")
    set(SYSCONFIG_ROOT "$ENV{SYSCONFIG_ROOT}" CACHE PATH "SysConfig root directory" FORCE)
endif()

if(NOT TI_CLANG_ROOT)
    message(FATAL_ERROR "TI_CLANG_ROOT is not set. Export TI_CLANG_ROOT or pass -DTI_CLANG_ROOT=/path/to/ti-cgt-c29")
endif()

if(NOT MCU_SDK_ROOT)
    message(FATAL_ERROR "MCU_SDK_ROOT is not set. Export MCU_SDK_ROOT or pass -DMCU_SDK_ROOT=/path/to/f29h85x-sdk")
endif()

# Find the TI Clang compiler tools before project() so CMake uses them for language checks.
find_program(TI_CLANG_C_COMPILER
    NAMES c29clang
    HINTS "${TI_CLANG_ROOT}/bin"
    NO_DEFAULT_PATH
    REQUIRED
)

find_program(TI_CLANG_ASM_COMPILER
    NAMES c29clang
    HINTS "${TI_CLANG_ROOT}/bin"
    NO_DEFAULT_PATH
    REQUIRED
)

find_program(TI_CLANG_AR
    NAMES c29ar
    HINTS "${TI_CLANG_ROOT}/bin"
    NO_DEFAULT_PATH
    REQUIRED
)

set(CMAKE_C_COMPILER "${TI_CLANG_C_COMPILER}" CACHE FILEPATH "C compiler" FORCE)
set(CMAKE_ASM_COMPILER "${TI_CLANG_ASM_COMPILER}" CACHE FILEPATH "ASM compiler" FORCE)
set(CMAKE_AR "${TI_CLANG_AR}" CACHE FILEPATH "Archiver" FORCE)

# Exclude the generated build tree before project() emits compiler-identification
# sources that CCS Theia could otherwise discover and add to the project build.
file(WRITE "${CMAKE_BINARY_DIR}/.exclude" "")

# Project definition
project(bl-f29h85x-ecat
    VERSION 1.0.0
    LANGUAGES C ASM
    DESCRIPTION "F29H85x CPU1 SOES (SOEM) Embedded Application"
)

# ==============================================================================
# Configuration Options
# ==============================================================================

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Option to use SysConfig generated files
option(USE_SYSCFG "Use TI SysConfig generated files" ON)

# Option to specify output format (ELF or BIN)
option(CREATE_HEX "Create HEX output file" OFF)
option(CREATE_BIN "Create BIN output file" ON)
option(USE_DUMMY_CERT "Use dummy certificate post-build flow" ON)
set(TARGET_CONFIG "FLASH" CACHE STRING "Target memory configuration")
set_property(CACHE TARGET_CONFIG PROPERTY STRINGS FLASH RAM)

# External workspace paths mirrored from CCS project variables.
set(GIT_SOES_ROOT "/home/amargan/work/code/firmware/SOES" CACHE PATH "Path to SOES repository")
set(GIT_UC_TEST_ROOT "/home/amargan/work/code/firmware/uc_test" CACHE PATH "Path to uc_test repository")

find_program(HOST_PYTHON NAMES python3 python)

set(FLASH_LOAD_ADDR "0x10001000" CACHE STRING "FLASH boot load address")
set(RAM_LOAD_ADDR "0x200E1000" CACHE STRING "RAM boot load address")
set(DUMMY_CERT_FLASH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dummy_cert_crypto_unlock_flash.cert" CACHE FILEPATH "Dummy FLASH cert file")
set(DUMMY_CERT_RAM_FILE "${CMAKE_CURRENT_SOURCE_DIR}/dummy_cert_crypto_unlock_ram.cert" CACHE FILEPATH "Dummy RAM cert file")
set(SIGNING_KEY_FILE "${MCU_SDK_ROOT}/tools/boot/signing/mcu_gpkey.pem" CACHE FILEPATH "Signing key for ROM image generation")
set(UPDATE_DUMMY_CERT_TOOL "${MCU_SDK_ROOT}/tools/misc/updateDummyCert.bin" CACHE FILEPATH "Dummy cert update helper")

# ==============================================================================
# Device Configuration
# ==============================================================================

set(DEVICE "F29H850TU9")
set(DEVICE_FAMILY "C2000")
set(CPU_VARIANT "CPU1")

# Add device definitions
add_compile_definitions(DEBUG CPU1 NO_PDI_HAL_TEST ESC_DEBUG PRINTF_REDEFINE)
if(TARGET_CONFIG STREQUAL "FLASH")
    add_compile_definitions(_FLASH)
    set(BOOT_MODE "FLASH")
    set(BOOT_LOAD_ADDR "${FLASH_LOAD_ADDR}")
    set(DUMMY_CERT_FILE "${DUMMY_CERT_FLASH_FILE}")
else()
    add_compile_definitions(RAM PDI_HAL_TEST)
    set(BOOT_MODE "RAM")
    set(BOOT_LOAD_ADDR "${RAM_LOAD_ADDR}")
    set(DUMMY_CERT_FILE "${DUMMY_CERT_RAM_FILE}")
endif()

if(USE_DUMMY_CERT)
    set(DUMMY_CERT_FLAG 1)
else()
    set(DUMMY_CERT_FLAG 0)
endif()

# ==============================================================================
# Source Files
# ==============================================================================

# Root project source files
set(PROJECT_SOURCES
    main.c
    boot_f29h85x.c
    boot_image.c
    ethercat_subdevice_cpu1_hal.c
    flash_utils.c
    foe_utils.c
    "${MCU_SDK_ROOT}/examples/device_support/source/codestartbranch.asm"
)

# Linked CCS project files that live outside this repository.
set(SOES_SOURCES)
set(SOES_TEST_SOURCES)

if(EXISTS "${GIT_SOES_ROOT}/soes/ecat_slv.c")
    list(APPEND SOES_SOURCES
        "${GIT_SOES_ROOT}/soes/ecat_slv.c"
        "${GIT_SOES_ROOT}/soes/esc.c"
        "${GIT_SOES_ROOT}/soes/esc_coe.c"
        "${GIT_SOES_ROOT}/soes/esc_foe.c"
        "${GIT_SOES_ROOT}/soes/hal/advr_esc/esc_hw_f29x.c"
    )
else()
    message(WARNING "SOES sources not found under GIT_SOES_ROOT='${GIT_SOES_ROOT}'. Build will exclude linked CCS SOES files.")
endif()

if(EXISTS "${GIT_UC_TEST_ROOT}/common_src/soes_test/globals.c")
    list(APPEND SOES_TEST_SOURCES
        "${GIT_UC_TEST_ROOT}/common_src/soes_test/globals.c"
        "${GIT_UC_TEST_ROOT}/common_src/soes_test/objectlist.c"
        "${GIT_UC_TEST_ROOT}/common_src/soes_test/params.c"
        "${GIT_UC_TEST_ROOT}/common_src/soes_test/soes_hook.c"
    )
else()
    message(WARNING "soes_test sources not found under GIT_UC_TEST_ROOT='${GIT_UC_TEST_ROOT}'.")
endif()

# CCS excludes device/driverlib sources and links prebuilt driverlib.lib.
set(DEVICE_SOURCES)

# SysConfig generated sources
set(SYSCFG_OUT_DIR "${CMAKE_BINARY_DIR}/syscfg")
if(USE_SYSCFG)
    set(SYSCFG_SOURCES
        "${SYSCFG_OUT_DIR}/board.c"
        "${SYSCFG_OUT_DIR}/device.c"
        "${SYSCFG_OUT_DIR}/ti_drivers_config.c"
        "${SYSCFG_OUT_DIR}/ti_dpl_config.c"
    )
    # These are generated at build-time by SysConfig.
    set_source_files_properties(${SYSCFG_SOURCES} PROPERTIES GENERATED TRUE)
else()
    set(SYSCFG_SOURCES)
endif()

set(ALL_SOURCES 
    ${PROJECT_SOURCES}
    ${SOES_SOURCES}
    ${SOES_TEST_SOURCES}
    ${DEVICE_SOURCES}
    ${SYSCFG_SOURCES}
)

# ==============================================================================
# Include Directories
# ==============================================================================

set(PROJECT_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/device
    ${CMAKE_CURRENT_SOURCE_DIR}/device/driverlib/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/device/driverlib/ccs
    ${MCU_SDK_ROOT}/source
    ${MCU_SDK_ROOT}/source/security
    ${MCU_SDK_ROOT}/source/driverlib
    ${MCU_SDK_ROOT}/source/calibration/hrpwm/include
    ${MCU_SDK_ROOT}/source/bitfields
    ${MCU_SDK_ROOT}/examples/device_support/include
    ${MCU_SDK_ROOT}/source/rtlibs/dcl
    ${MCU_SDK_ROOT}/source/rtlibs/dcl/common
    ${MCU_SDK_ROOT}/source/rtlibs/dcl/df
    ${MCU_SDK_ROOT}/source/rtlibs/dcl/misc
    ${MCU_SDK_ROOT}/source/rtlibs/dcl/pi
    ${MCU_SDK_ROOT}/source/rtlibs/dcl/pid
    ${MCU_SDK_ROOT}/source/rtlibs/dsp/fpu/fpu32/fft
    ${MCU_SDK_ROOT}/source/rtlibs/dsp/fpu/fpu32/fir
    ${MCU_SDK_ROOT}/source/rtlibs/dsp/fpu/fpu32/iir
    ${MCU_SDK_ROOT}/source/rtlibs/fastmath/crc
    ${MCU_SDK_ROOT}/source/rtlibs/fastmath/matrix
    ${MCU_SDK_ROOT}/source/rtlibs/fastmath/binary_lut_search
    ${MCU_SDK_ROOT}/source/rtlibs/iqmath
    ${MCU_SDK_ROOT}/source/kernel/freertos/Source/include
    ${MCU_SDK_ROOT}/source/kernel/freertos/Source/portable/CCS/C2000_C29x
    ${MCU_SDK_ROOT}/source/flash_api/include/FlashAPI
    ${MCU_SDK_ROOT}/source/kernel/dpl
    ${MCU_SDK_ROOT}/source/pmic
    ${MCU_SDK_ROOT}/source/sdl
    ${TI_CLANG_ROOT}/include
    ${GIT_SOES_ROOT}
    ${GIT_SOES_ROOT}/soes/include/sys/clang
    ${GIT_UC_TEST_ROOT}/common_src/soes_test/include
)

if(USE_SYSCFG)
    list(APPEND PROJECT_INCLUDE_DIRS
        ${SYSCFG_OUT_DIR}
    )
endif()

# ==============================================================================
# Compiler Flags
# ==============================================================================

set(CMAKE_C_FLAGS_DEBUG "-g -w -O1 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-g -w -O2 -DDEBUG")
set(CMAKE_ASM_FLAGS_DEBUG "-g -w -O1 -DDEBUG")
set(CMAKE_ASM_FLAGS_RELEASE "-g -w -O2 -DDEBUG")

# ==============================================================================
# Linker Flags and Script
# ==============================================================================

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/f29h85x_flash_lnk.cmd")
if(TARGET_CONFIG STREQUAL "RAM")
    set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/f29h85x_ram_lnk.cmd")
endif()

# ==============================================================================
# Create Executable
# ==============================================================================

add_executable(${PROJECT_NAME}.out
    ${ALL_SOURCES}
)

# Set include directories for the executable
target_include_directories(${PROJECT_NAME}.out PRIVATE
    ${PROJECT_INCLUDE_DIRS}
)

# Add compile options to target
target_compile_options(${PROJECT_NAME}.out PRIVATE
    -Qunused-arguments
    -std=c1x
)

target_link_options(${PROJECT_NAME}.out PRIVATE
    -Wl,-m${PROJECT_NAME}.map
    -Wl,--heap_size=0x400
    -Wl,--stack_size=0x400
    -Wl,-i${TI_CLANG_ROOT}/lib
    -Wl,--reread_libs
    -Wl,--diag_wrap=off
    -Wl,--display_error_number
    -Wl,--warn_sections
    -Wl,--xml_link_info=${PROJECT_NAME}_linkInfo.xml
    -Wl,--entry_point=code_start
    -Wl,--rom_model
    -Wl,-i${MCU_SDK_ROOT}/source/driverlib/ccs
    -Wl,-i${MCU_SDK_ROOT}/source/flash_api/lib
    -Wl,-i${MCU_SDK_ROOT}/source/calibration/hrpwm/lib
    -Wl,-i${MCU_SDK_ROOT}/source/security/ccs
    -Wl,-i${MCU_SDK_ROOT}/source/kernel/nortos/ccs
    -Wl,-i${MCU_SDK_ROOT}/source/sdl/ccs
)

if(TARGET_CONFIG STREQUAL "FLASH")
    target_link_options(${PROJECT_NAME}.out PRIVATE
        -Wl,--define=_FLASH
    )
else()
    target_link_options(${PROJECT_NAME}.out PRIVATE
        -Wl,--define=RAM
    )
endif()

# Link dependencies (if any libraries need to be linked)
target_link_libraries(${PROJECT_NAME}.out PRIVATE
    "${LINKER_SCRIPT}"
    "${MCU_SDK_ROOT}/source/driverlib/ccs/driverlib.lib"
    "${MCU_SDK_ROOT}/source/flash_api/lib/F29H85x_NWFlashAPI_v21.00.00.00.lib"
    "${MCU_SDK_ROOT}/source/calibration/hrpwm/lib/sfo.lib"
    "${MCU_SDK_ROOT}/source/security/ccs/security_drivers.lib"
    "${MCU_SDK_ROOT}/source/kernel/nortos/ccs/dpl_nortos.lib"
    "${MCU_SDK_ROOT}/source/sdl/ccs/sdl_debug.lib"
)

# Set linker language
set_target_properties(${PROJECT_NAME}.out PROPERTIES
    LINKER_LANGUAGE C
    SUFFIX ""
)

# ==============================================================================
# Post-Build Processing
# ==============================================================================

# Create HEX file
if(CREATE_HEX)
    add_custom_command(TARGET ${PROJECT_NAME}.out POST_BUILD
        COMMAND ${TI_CLANG_ROOT}/bin/c29objcopy -O ihex $<TARGET_FILE:${PROJECT_NAME}.out> ${PROJECT_NAME}.hex
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating HEX file..."
    )
endif()

# Create BIN file
if(CREATE_BIN)
    add_custom_command(TARGET ${PROJECT_NAME}.out POST_BUILD
        COMMAND ${TI_CLANG_ROOT}/bin/c29objcopy --remove-section=cert -O binary $<TARGET_FILE:${PROJECT_NAME}.out> ${PROJECT_NAME}.bin
        COMMAND /bin/sh -c "[ ${DUMMY_CERT_FLAG} -eq 0 ] && \"${HOST_PYTHON}\" \"${MCU_SDK_ROOT}/tools/boot/signing/mcu_rom_image_gen.py\" --image-bin \"${PROJECT_NAME}.bin\" --core C29 --swrv 1 --loadaddr ${BOOT_LOAD_ADDR} --sign-key \"${SIGNING_KEY_FILE}\" --out-image \"${PROJECT_NAME}_cert.bin\" --device f29h85x --boot ${BOOT_MODE} --img_integ no || :"
        COMMAND /bin/sh -c "[ ${DUMMY_CERT_FLAG} -eq 0 ] && \"${TI_CLANG_ROOT}/bin/c29objcopy\" --update-section cert=C29-cert-pad.bin \"${PROJECT_NAME}.out\" \"${PROJECT_NAME}.out\" || :"
        COMMAND /bin/sh -c "[ ${DUMMY_CERT_FLAG} -eq 1 ] && chmod +x \"${UPDATE_DUMMY_CERT_TOOL}\" && \"${UPDATE_DUMMY_CERT_TOOL}\" \"${DUMMY_CERT_FILE}\" \"${PROJECT_NAME}.bin\" || :"
        COMMAND /bin/sh -c "[ ${DUMMY_CERT_FLAG} -eq 1 ] && cat \"${DUMMY_CERT_FILE}\" \"${PROJECT_NAME}.bin\" > \"${PROJECT_NAME}_cert.bin\" || :"
        COMMAND /bin/sh -c "[ ${DUMMY_CERT_FLAG} -eq 1 ] && \"${TI_CLANG_ROOT}/bin/c29objcopy\" --update-section cert=\"${DUMMY_CERT_FILE}\" \"${PROJECT_NAME}.out\" \"${PROJECT_NAME}.out\" || :"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        VERBATIM
        COMMENT "Creating BIN file..."
    )
endif()

# Print build information
add_custom_command(TARGET ${PROJECT_NAME}.out POST_BUILD
    COMMAND ${TI_CLANG_ROOT}/bin/c29size $<TARGET_FILE:${PROJECT_NAME}.out>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Memory usage:"
)

# ==============================================================================
# SysConfig Integration Target
# ==============================================================================

if(USE_SYSCFG)
    add_custom_command(
        OUTPUT ${SYSCFG_SOURCES}
        COMMAND ${SYSCONFIG_ROOT}/sysconfig_cli.sh
            -s ${MCU_SDK_ROOT}/.metadata/sdk.json
            --script ${CMAKE_CURRENT_SOURCE_DIR}/f29h85x_cpu1_soes.syscfg
            -o ${SYSCFG_OUT_DIR}
            --compiler ticlang
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/f29h85x_cpu1_soes.syscfg
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Regenerating SysConfig files..."
    )

    # Add a custom target to regenerate SysConfig files
    add_custom_target(generate_syscfg DEPENDS ${SYSCFG_SOURCES})
    
    # Make the main executable depend on syscfg generation
    add_dependencies(${PROJECT_NAME}.out generate_syscfg)
endif()

# Keep the CCS pre-build metadata generation behavior when the script exists.
set(BUILD_INFO_SCRIPT "${GIT_UC_TEST_ROOT}/common_src/gen_build_info.sh")
set(BUILD_INFO_TARGET_DIR "${GIT_UC_TEST_ROOT}/common_src/soes_test")
set(BUILD_INFO_HEADER "${BUILD_INFO_TARGET_DIR}/include/build_info.h")

if(EXISTS "${BUILD_INFO_SCRIPT}")
    add_custom_target(generate_build_info
        COMMAND "${BUILD_INFO_SCRIPT}" "${BUILD_INFO_TARGET_DIR}"
        COMMENT "Generating build metadata for soes_test..."
        VERBATIM
    )
    add_dependencies(${PROJECT_NAME}.out generate_build_info)
endif()

# ==============================================================================
# Build Summary
# ==============================================================================

message(STATUS "")
message(STATUS "F29H85x EtherCAT FoE Bootloader Build Configuration")
message(STATUS "=====================================")
message(STATUS "Build Type:              ${CMAKE_BUILD_TYPE}")
message(STATUS "Device:                  ${DEVICE}")
message(STATUS "CPU:                     ${CPU_VARIANT}")
message(STATUS "TI Clang Root:           ${TI_CLANG_ROOT}")
message(STATUS "MCU SDK Root:            ${MCU_SDK_ROOT}")
message(STATUS "Use SysConfig:           ${USE_SYSCFG}")
message(STATUS "Create HEX:              ${CREATE_HEX}")
message(STATUS "Create BIN:              ${CREATE_BIN}")
message(STATUS "Linker Script:           ${LINKER_SCRIPT}")
message(STATUS "")
