# Build Guide

This project uses TI C29 Clang (`c29clang`) and CMake.

## Requirements

- CMake 3.20+
- TI C29 compiler (for example `ti-cgt-c29_2.2.0.LTS`)
- F29H85x SDK
- SysConfig

## Configure

```bash
cmake -B build -S . \
  -DTI_CLANG_ROOT=/home/amargan/ti/ccs2100/ccs/tools/compiler/ti-cgt-c29_2.2.0.LTS \
  -DMCU_SDK_ROOT=/home/amargan/ti/f29h85x-sdk_1_02_01_00 \
  -DSYSCONFIG_ROOT=/home/amargan/ti/sysconfig_1.27.1
```

## Build

```bash
cmake --build build -j
```

## Common Options

| Option | Default | Notes |
|---|---|---|
| `CMAKE_BUILD_TYPE` | `Debug` | Use `Release` if needed |
| `TARGET_CONFIG` | `FLASH` | `FLASH` or `RAM` |
| `USE_SYSCFG` | `ON` | Generates files into `build/syscfg` |
| `CREATE_BIN` | `ON` | Creates `.bin` from `.out` |
| `CREATE_HEX` | `OFF` | Creates `.hex` |
| `GIT_SOES_ROOT` | `/home/amargan/work/code/firmware/SOES` | CCS linked source root |
| `GIT_UC_TEST_ROOT` | `/home/amargan/work/code/firmware/uc_test` | CCS linked test/hook root |

Example:

```bash
cmake -B build -S . \
  -DTARGET_CONFIG=RAM \
  -DCMAKE_BUILD_TYPE=Release
```

## Outputs

- `build/f29h85x_cpu1_soes.out`
- `build/f29h85x_cpu1_soes.bin` (if `CREATE_BIN=ON`)
- `build/f29h85x_cpu1_soes.hex` (if `CREATE_HEX=ON`)
- `build/f29h85x_cpu1_soes.map`

## SysConfig Only

```bash
cmake --build build --target generate_syscfg
```

## SysConfig Path Setup

Set `SYSCONFIG_ROOT` to the SysConfig install folder so CMake can run `sysconfig_cli.sh`.

Configure example:

```bash
cmake -B build -S . \
  -DTI_CLANG_ROOT=/home/amargan/ti/ccs2100/ccs/tools/compiler/ti-cgt-c29_2.2.0.LTS \
  -DMCU_SDK_ROOT=/home/amargan/ti/f29h85x-sdk_1_02_01_00 \
  -DSYSCONFIG_ROOT=/home/amargan/ti/sysconfig_1.27.1
```

Typical paths:

- Linux: `/home/<user>/ti/sysconfig_*`
- CCS bundled: `<ccs>/ccs/utils/sysconfig_*`

Verify:

```bash
ls "$SYSCONFIG_ROOT/sysconfig_cli.sh"
```

Disable SysConfig generation:

```bash
cmake -B build -S . -DUSE_SYSCFG=OFF
```

## Troubleshooting

- Compiler not found:

```bash
ls "$TI_CLANG_ROOT/bin/c29clang"
```

- Reconfigure cleanly:

```bash
rm -rf build
cmake -B build -S . <same options>
```

- If external linked paths differ from your machine, pass:

```bash
-DGIT_SOES_ROOT=/your/path/SOES -DGIT_UC_TEST_ROOT=/your/path/uc_test
```

## Implementation Notes

Current CMake behavior is aligned to the CCS project settings:

- Uses TI C29 toolchain (`c29clang`, `c29ar`).
- Generates SysConfig files during build into `build/syscfg`.
- Includes external linked CCS sources from SOES and `uc_test/common_src/soes_test`.
- Excludes local `device/driverlib/*.c` compilation and links prebuilt SDK `driverlib.lib`.
- Mirrors CCS FLASH linker search paths and additional SDK libs.
