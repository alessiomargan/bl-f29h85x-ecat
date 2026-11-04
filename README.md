# f29h85x_cpu1_soes

EtherCAT CPU1 firmware for F29H85x, with CCS project files and a CMake build path aligned to the CCS Clang (C29) configuration.

## Quick Build

```bash
cmake -B build -S . \
	-DTI_CLANG_ROOT=/home/amargan/ti/ccs2100/ccs/tools/compiler/ti-cgt-c29_2.2.0.LTS \
	-DMCU_SDK_ROOT=/home/amargan/ti/f29h85x-sdk_1_02_01_00 \
	-DSYSCONFIG_ROOT=/home/amargan/ti/sysconfig_1.27.1

cmake --build build -j
```

## Main Docs

- [docs/CMAKE_INDEX.md](docs/CMAKE_INDEX.md): docs navigation
- [docs/BUILD.md](docs/BUILD.md): full build instructions and options
- [docs/CMAKE_QUICK_START.md](docs/CMAKE_QUICK_START.md): minimal command reference
