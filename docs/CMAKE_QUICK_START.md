# CMake Quick Start

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

## Useful Variants

```bash
cmake -B build -S . -DTARGET_CONFIG=RAM
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --target generate_syscfg
```

## Output Files

- `build/f29h85x_cpu1_soes.out`
- `build/f29h85x_cpu1_soes.bin`
- `build/f29h85x_cpu1_soes.map`

For details and troubleshooting, use [BUILD.md](BUILD.md).
