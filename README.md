# bl-f29h85x-ecat

EtherCAT File-over-EtherCAT (FoE) recovery bootloader for the TI F29H85x
CPU1 on the controlCARD. It uses the integrated EtherCAT SubDevice controller,
SOES, and the F29H85x Flash API to install and validate `f29h85-soes` images.

The bootloader is a separate executable and must be programmed once with a
debug probe. Normal application updates are then delivered through FoE.

## Repository setup

Clone with the SOES submodule:

```bash
git clone --recurse-submodules <bootloader-repository-url>
cd bl-f29h85x-ecat
```

For an existing clone:

```bash
git submodule update --init --recursive
```

The SOES revision is pinned. `tools/check_foe_stack.py` verifies the reviewed
FoE completion and error-handling sources before a CCS build.

Bootloader-owned C sources and headers live in `src/`. The SysConfig input,
linker command files, build wrappers, tests, and target configuration remain at
the project root in their conventional directories.

## Build

### Code Composer Studio

Import the project and build configuration `BLDR_CC`. This is the only board
configuration: F29H85x is used with the controlCARD.

The CCS post-build creates:

- `bl-f29h85x-ecat.bin`: flat bootloader binary
- `bl-f29h85x-ecat_cert.bin`: ROM-bootable certificate plus image
- `bl-f29h85x-ecat.out`: debugger/programmer executable

Program the bootloader `.out` or certified image with CCS/UniFlash before
installing an application.

### CMake with TI C29 Clang

The standalone build uses the same sources, SysConfig input, linker command
file, compiler, and pinned submodule as CCS:

```bash
make build
```

Paths can be overridden when required:

```bash
make build \
  TI_CLANG_ROOT=/path/to/ti-cgt-c29 \
  MCU_SDK_ROOT=/path/to/f29h85x-sdk \
  SYSCONFIG_ROOT=/path/to/sysconfig
```

## Flash layout

| Region | Address range | Purpose |
|---|---:|---|
| ROM certificate | `0x10000000-0x10000FFF` | Secure/ROM boot metadata |
| Bootloader | `0x10001000-0x1003FFFF` | `BLDR_CC` executable |
| Application payload | `0x10040000-0x101FDFFF` | `f29h85-soes` program image |
| Image manifest | `0x101FE000-0x101FEFFF` | Length, target, and CRC |
| Commit sector | `0x101FF000-0x101FFFFF` | Written last after validation |
| Retained request | `0x200FFFF8-0x200FFFFF` | Warm application-to-bootloader request |

The application entry point is `0x10040000`. The bootloader rejects images
with a wrong target, address, length, header CRC, payload CRC, or commit marker.
The commit sector is erased before an update and programmed last, so an
interrupted transfer is not bootable.

## Update flow

The application writes the retained request and jumps to bootloader codestart.
The bootloader preserves the integrated ESC during this warm handoff, enters
EtherCAT Bootstrap, and accepts one file:

- filename: `f29h85x.bin`
- FoE password: `0xF290`
- application address: `0x10040000`

Create that file from the application repository with `make foe-package`.
The FoE stream contains a 32-byte manifest followed immediately by the actual
application bytes, with no full-slot padding. After reading the manifest, the
bootloader erases only the payload sectors required by its declared length.
After the final block, it validates the complete image and writes the commit
marker. An invalid or interrupted image keeps the device in recovery.

## Tests

```bash
python3 -m unittest -v tests/test_boot_image.py
python3 tools/check_foe_stack.py
```

The host tests check packer/validator agreement, payload corruption, truncated
transfers, and invalid input sizes.
