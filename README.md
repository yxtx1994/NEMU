# NEMU

NEMU(NJU Emulator) is a simple but complete full-system emulator designed for teaching purpose.
Currently it supports x86, mips32, and riscv32.
To build programs run above NEMU, refer to the [AM project](https://github.com/NJU-ProjectN/abstract-machine).

This is the NEMU Version for agile Design Space Exploration(DSE) framework only, for Nanhu-G Core.

The main features of NEMU include
* a small monitor with a simple debugger
  * single step
  * register/memory examination
  * expression evaluation without the support of symbols
  * watch point
  * differential testing with reference design (e.g. QEMU)
  * snapshot
* CPU core with support of most common used instructions
  * x86
    * real mode is not supported
    * x87 floating point instructions are not supported
  * mips32
    * CP1 floating point instructions are not supported
  * riscv32
    * only RV32IM
* memory
* paging
  * TLB is optional (but necessary for mips32)
  * protection is not supported
* interrupt and exception
  * protection is not supported
* 5 devices
  * serial, timer, keyboard, VGA, audio
  * most of them are simplified and unprogrammable
* 2 types of I/O
  * port-mapped I/O and memory-mapped I/O

## Howto

### Run OpenSBI and Linux

**Steps below to run OpenSBI and Linux use source code in OpenXiangShan**

1. Compile a Linux kernel, with proper SD card driver integrated if you want to run Debian or Fedora. Currently Kernel v4.18 is verified to work.
2. Convert `vmlinux` to binary format using `objcopy`.
3. Compile OpenSBI using `build_linux.sh` where vmlinux and dtb path may need a modification. dtb can be compiled with dts files located in `dts` subdirectory of OpenSBI.
4. Compile NEMU intepreter. You may want to change default sdcard image path in `src/devices/sdcard.c` to boot Debian or Fedora.
5. launch NEMU intepreter and load `fw_payload.bin` generated by OpenSBI. e.g. `./build/riscv64-nemu-interpreter ~/Xiangshan_Linux/opensbi/build/platform/generic/firmware/fw_payload.bin`
6. If you are using a `vmlinux` with initramfs, you will likely be greeted with a `Hello`, otherwise you may see startup logs and finally a login prompt from Debian or Fedora if SD card is configured properly.

### Run baremetal app

```
git submodule update --init ready-to-run
./build/riscv64-nemu-interpreter -b ./ready-to-run/coremark-2-iteration.bin
```

### Prepare workloads

**Link your bbl or baremetal app to 0x800a0000**

We use [gcpt restorer](https://github.com/OpenXiangShan/NEMU/tree/master/resource/gcpt_restore)
to restore architectural checkpoints, which is a piece of assembly code.
We put gcpt restorer at 0x80000000 and put bbl or baremetal app at 0x800a0000.
So users should link bbl or baremetal app to 0x800a0000 instead of 0x80000000.
Users can build the [bbl here](https://github.com/OpenXiangShan/riscv-pk/commits/noop-take-cpt) which has been linked to 0x0x800a0000.
A prebuilt bbl linked to 0x800a0000 is also shipped in ready-to-run.

### Take uniform checkpoints and restore checkpoints

Take uniform checkpoints (checkpointing every N instructions).

```shell
./build/riscv64-nemu-interpreter \
    --cpt-interval 10000000 -u -b \
    -D output_top \
    -C test \
    -w linux \
    -r ./resource/gcpt_restore/build/gcpt.bin \
    --dont-skip-boot\
    -I 11000000 ./ready-to-run/linux-0xa0000.bin
```

Restore checkpoints (example):
```shell
./build/riscv64-nemu-interpreter -b\
    --restore \
    ./output_top/test/linux/0/_10000003_.gz
```

### Generate BBV and take simpoint-directed checkpoints

This feature has NOT been tested yet, and might be broken.
