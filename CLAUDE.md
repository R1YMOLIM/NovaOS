# NovaOS — Project Analysis for Claude

## Overview

**NovaOS** is a hobby x86_64 operating system "built from scratch". It currently boots via UEFI, hands control to a freestanding C kernel, and demonstrates a basic framebuffer graphics pipeline plus a buddy allocator for physical memory. The codebase is small, single-developer, and explicitly educational — every commit message is a status note, not a release.

The project is in an early stage: the bootloader exists, the kernel stub runs, the memory subsystem has a working page allocator, and a custom Python+Ninja build system drives the whole thing.

---

## Repository layout

```
NovaOS/
├── arch/x86_64/
│   ├── boot/
│   │   ├── main_boot.c        # UEFI bootloader (EfiMain) — currently DELETED (D) in working tree
│   │   └── uefi/               # UEFI type/protocol headers (also staged for deletion)
│   └── io.c                    # empty
├── boot/                       # (deleted) legacy boot directory
├── include/                    # shared headers between boot and kernel
│   ├── bootloader_info.h       # boot_memory_info_t, efi_memory_descriptor_t
│   ├── memory.h                # re-export of efi_memory_descriptor_t / boot_memory_info_t
│   └── video.h                 # boot_video_info_t (framebuffer descriptor)
├── kernel/                     # freestanding x86_64 ELF kernel
│   ├── main_microkernel.c      # _start(sysv_abi, noreturn)
│   ├── memory.c                # buddy allocator (kalloc/kfree/init_memory)
│   ├── video.c                 # framebuffer drawing primitives
│   ├── microkernel.h           # public kernel API + types
│   ├── types.h                 # uintN_t, phys_addr_t, virt_addr_t, NULL, bool
│   └── linker.ld               # kernel link script (_start @ 0x100000)
├── tools/
│   ├── nova_build.py           # custom TOML→Ninja generator
│   └── start_nova_os.sh        # QEMU launcher with OVMF
├── legacy/legacy.txt           # old code (gitignored)
├── nova_build.toml             # build config (project, toolchain, targets)
├── build.ninja                 # generated
├── build/                      # generated artifacts (EFI/BOOT/BOOTX64.EFI, kernel, my_vars.fd)
├── .clang-format               # LLVM base, 2-space indent, 100-col
├── .gitignore
└── README.md                   # "Os From Scratch"
```

---

## Boot chain

1. **UEFI firmware (OVMF)** loads `build/EFI/BOOT/BOOTX64.EFI`.
2. **`arch/x86_64/boot/main_boot.c`** (EfiMain) — locates the kernel ELF on the FAT volume, parses the UEFI memory map, sets up GOP framebuffer, then `ExitBootServices` and jumps to the kernel entry `_start`. *Currently this file is staged for deletion in the working tree (see "Pending changes" below).*
3. **Kernel `_start`** (in `kernel/main_microkernel.c`) receives a `boot_loader_info_t *` (containing `boot_video_info_t` + `boot_memory_info_t`), initializes subsystems, runs a demo, then `hlt` loops.

---

## Architecture-specific notes

- **Target**: x86_64 only (`arch = "x86_64"` in `nova_build.toml`).
- **Kernel entry ABI**: `__attribute__((sysv_abi, noreturn))` — calling convention is the standard System V AMD64, not UEFI/MS x64.
- **Kernel base**: `_start` is linked to `0x100000` (the conventional 1 MiB mark) via `kernel/linker.ld`. Two `PT_LOAD` PHDRs: `.text` (RX) and `.data` (RW).
- **Kernel halt**: spinning `hlt` in both `_start` failure paths and the main loop.
- **Stack / heap**: no own initialization yet — relies on whatever the bootloader leaves in place after `ExitBootServices`.

---

## Kernel subsystems

### Video (`kernel/video.c`, `include/video.h`)
- Backed by `boot_video_info_t` (framebuffer: `base_address`, `width`, `height`, `pixels_per_scan_line`, `buffer_size`).
- Primitives: `draw_pixel`, `fill_screen`, `draw_line` (H/V only), `draw_rectangle`.
- Pixel format: treats each pixel as a 32-bit ARGB/XRGB word written directly to the framebuffer. No format conversion (the earlier `convert_pixel_format` work has been removed).
- `g_video` is a static NULL-initialized pointer set by `init_video`; all primitives guard against NULL.

### Memory (`kernel/memory.c`, `include/memory.h`, `kernel/microkernel.h`)
- Walks the UEFI memory map, picks the largest region of `EfiConventionalMemory` (type 7).
- Builds a **buddy allocator** metadata tree (array of `node_state_t`: `NODE_FREE=0`, `NODE_SPLIT=1`, `NODE_ALLOCATED=2`, `NODE_FAILED=3`).
- Tree placement: at the very start of the chosen free region (phys base). The metadata tree pages are *consumed* from that region and skipped before the allocator hands out the rest.
- Power-of-two rounding: `max_pages` is rounded up to the next power of two for a balanced tree (`tree_nodes = 2*N - 1`).
- API:
  - `kalloc(size_t req_pages)` returns a tree index (0 = root) on success, `NODE_FAILED` otherwise. **It returns an index, not a virtual/physical address** — callers treat non-zero as success.
  - `kfree(size_t index)` flips the node to `FREE` and walks up merging buddies.
  - `init_memory(const boot_memory_info_t *)` performs the walk and self-init.

### Types (`kernel/types.h`)
- Hand-rolled `uintN_t`, `intN_t`, `size_t`, `uintptr_t`, `bool`/`true`/`false`, `NULL`, `phys_addr_t`, `virt_addr_t`. No `<stdint.h>` because the kernel is freestanding.
- `int128_t` / `uint128_t` only on x86_64.

### Microkernel header (`kernel/microkernel.h`)
- Aggregates the public kernel API (video + memory).
- Forwards `boot_video_info_t` and `boot_memory_info_t` via includes.

---

## Build system (`tools/nova_build.py` + `nova_build.toml`)

### Toolchain
- `clang` for everything.
- `ld.lld` for the kernel ELF (GNU ld flags).
- `lld-link` for the UEFI bootloader (MSVC-style flags: `-entry:EfiMain`, `-subsystem:efi_application`, `-dll`).
- Common flags: `-ffreestanding -fno-stack-protector -fno-builtin -fno-pic -fno-pie -mno-red-zone -Wall -Wextra -Iinclude`.

### Targets
- `boot` — `type = "uefi"`, sources from `arch/${arch}/boot`, target triple `x86_64-unknown-windows`, output `build/EFI/BOOT/BOOTX64.EFI`.
- `kernel` — `type = "elf"`, sources from `kernel/`, `-m64`, linker script `kernel/linker.ld`, output `build/kernel`.

### NovaBuild flow
1. Reads `nova_build.toml` via `tomllib`.
2. Substitutes `${arch}` and `${project}` placeholders.
3. Globs `*.c` under each target's `dirs` (and optional `sources`/`exclude`).
4. Generates `build.ninja` with `cc` (compile), `link_gnu` (ELF), `link_msvc` (UEFI) rules.
5. After a successful `ninja` run, also generates `compile_commands.json` via `ninja -t compdb cc` for clangd.

### CLI
- `python tools/nova_build.py` — generate + build.
- `python tools/nova_build.py clean` — wipe `build/`, `build.ninja`, `compile_commands.json`.

### Run
- `tools/start_nova_os.sh` — locates OVMF_CODE/VARS firmware, copies `my_vars.fd` on first run, launches QEMU with KVM, pflash firmware, FAT disk from `build/`, `-d int,cpu_reset,guest_errors -no-reboot -no-shutdown`, `-serial stdio`, `q35` machine, host CPU, no network. Logs go to `qemu.log`.

---

## Code style (`.clang-format`)

- LLVM base, C "Latest".
- 2-space indent, 2-space continuation indent, 2-col tabs (`UseTab: Never`).
- 100-col line limit.
- `Attach` braces, no short blocks/functions/if/loops on one line.
- `AlignAfterOpenBracket: Align`, `AlignOperands: Align`, `AlignTrailingComments: true`.
- `SortIncludes: CaseSensitive`, `IncludeBlocks: Regroup`.
- `SeparateDefinitionBlocks: Always` — blank line between function definitions.

Observed in the code: 2-space indent, `lower_snake_case` for functions, `UPPER_SNAKE_CASE` for UEFI types (`UINT16`, `VOID`, `EFI_GUID`), `lower_snake_case` for `node_state_t` enum values inside `enum { ... }` literals, `static` for file-local helpers, includes sorted (own header first, then `<…>`).

Commit messages: lower-case, `<area>: <imperative description>` (e.g. `kernel: added kfree function in memory`). Most recent ones are terse status updates.

---

## Pending changes (working tree)

`git status` shows that the boot subsystem is being removed/refactored:

- **Deleted from the index** (`D`): `boot/main_boot.c`, `boot/uefi/{boot_services,entry,system_table,runtime_services,types}.h`, `boot/uefi/protocols/{console,load_image,media}.h`.
- **Untracked**: `arch/` (the new home for boot code under `arch/x86_64/boot/`).
- **Modified**: `CLAUDE.md`, `nova_build.toml`, `tools/nova_build.py`.

Interpretation: the UEFI bootloader is migrating from `boot/uefi/` to `arch/x86_64/boot/uefi/`, and the build config + NovaBuild script are being updated to match. `git rm` has already been staged for the old paths, but the migration is incomplete — `arch/x86_64/boot/main_boot.c` still `#include`s the old `boot/uefi/...` paths (e.g. `#include "uefi/boot_services.h"`), which will not resolve once the old tree is gone. The new `boot` target in `nova_build.toml` already points at `arch/${arch}/boot`, so the intent is clear but the include paths and the build yet to converge.

`kernel/` sources are unaffected by the migration.

---

## What is *not* here yet

- No interrupts/IDT, no PIC/PIC2 or APIC setup, no timer.
- No paging — the kernel runs at the physical identity mapped from the bootloader (everything is identity-mapped via UEFI).
- No userspace, no syscalls, no scheduler, no ELF loader in the kernel proper.
- No filesystem beyond the bootloader's read of `kernel` from the FAT volume.
- No ACPI, no PCI, no USB.
- No SMP / per-CPU state.
- No serial *kernel* output — the bootloader has a COM1 UART driver (`main_boot.c`), but the kernel does not.
- No tests, no CI configuration.

---

## How to work on this project

- **Build**: `python tools/nova_build.py` (or `clean` first to reset).
- **Run**: `tools/start_nova_os.sh` after a successful build. QEMU needs the `ovmf` package (`/usr/share/OVMF` etc.).
- **LSP**: `compile_commands.json` is regenerated after each build — point clangd at the project root.
- **Format**: project uses `.clang-format`; LLVM base, 2-space, 100-col.
- **Adding a kernel source**: drop it under `kernel/` and it'll be picked up automatically; no need to touch `nova_build.toml` unless you want a per-target exclusion.
- **Adding a new subsystem module**: forward-declare `init_*` in `kernel/main_microkernel.c` like `init_video` / `init_memory` are, then define it in its own `.c` and call it from `_start`. The kernel currently uses `extern` forward declarations to isolate modules — preserve that pattern.

---

## Quick reference — key files

| File | Role |
|---|---|
| `tools/nova_build.py` | TOML → Ninja generator, build driver |
| `nova_build.toml` | Project name, toolchain, target definitions |
| `kernel/linker.ld` | Kernel layout at `0x100000` |
| `kernel/main_microkernel.c` | Kernel `_start` (sysv_abi, noreturn) |
| `kernel/microkernel.h` | Public kernel API + shared types |
| `kernel/memory.c` | Buddy allocator + UEFI memory map walk |
| `kernel/video.c` | Framebuffer drawing primitives |
| `kernel/types.h` | Freestanding typedefs |
| `include/bootloader_info.h` | Hand-off struct between boot and kernel |
| `arch/x86_64/boot/main_boot.c` | UEFI bootloader entry (in migration) |
| `tools/start_nova_os.sh` | QEMU + OVMF launcher |
