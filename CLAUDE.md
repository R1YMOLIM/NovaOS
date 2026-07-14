# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Working rules (project owner)

These are the rules the project owner has set for how AI assistance is used in this repo. They apply to every session.

### What AI is allowed to do

- **Explain terminology and concepts** — what an `EFI_GUID` is, what "calling convention" means, what the difference is between boot services and runtime services, etc.
- **Point to documentation** — which spec section (§7.4, §13.4, etc.), which page, which table. The owner reads the actual spec.
- **Discuss architecture and tradeoffs** — "should I do X or Y", "what are the pros and cons". The owner decides.

### What AI is NOT allowed to do

- **Write code for the project** — no AI-generated code in `boot/`, `kernel/`, `include/`, or anywhere else that ends up in the repo. The owner writes every line themselves.
- **Generate large code blocks** — short snippets for illustration in conversation are fine if the owner treats them as explanations, not patches to copy. Anything that looks like "drop this in" should be rejected.
- **Be the source of a borrowed implementation** — if code in NovaOS comes from elsewhere (spec example, EDK II, gnu-efi, Linux, OSDev wiki, an old driver, etc.), the owner cites the source and understands why it was adapted. 

### Why

The owner wants to learn the territory themselves. The risk of AI-written code is not bugs (those are visible) — it's that the owner skips the step of *understanding what they're building*. The goal of this project is the understanding, not just a working OS. Code that the owner can't explain on a whiteboard doesn't help with that goal.

When AI is used only for explanation and orientation, the owner keeps authorship, knowledge, and the ability to defend every design decision.

### Other rules

1. **Verify against the UEFI Spec** — the spec PDF is at `/home/rim/Downloads/UEFI_Spec_Final_2.11.pdf` (2301 pages, Release 2.11). Any non-trivial UEFI detail (struct layouts, calling conventions, protocol signatures, handoff semantics) must be cross-checked against the spec, not taken on the AI's word alone.
2. **Understand every line** — code that the owner can't explain does not get committed.
3. **NovaOS sets its own bar** — don't import conventions from Linux, EDK II, gnu-efi, or other OSes just because they're the norm there. This is a from-scratch project; the owner decides what fits.
4. **Check sources** — for any code, fact, or example cited from elsewhere, the owner verifies origin. This applies to UEFI spec citations, references to Linux/EDK II/other-OS patterns, "well-known practices," and historical claims. AI suggestions can be confidently wrong about specifics (project names, version histories, "this was borrowed from X"), so anything concrete must be checked, not trusted on tone.

When unsure about a UEFI detail, prefer reading the spec over guessing. When the spec is silent or ambiguous, surface that explicitly rather than papering over it.

## Project

NovaOS — an OS written from scratch, booted via UEFI. Current state: a minimal UEFI bootloader stub that prints "Hello world! Hello NovaOS!" and halts. The target output is a single PE/COFF EFI application at `build/EFI/BOOT/BOOTX64.EFI`, runnable in QEMU with OVMF.

## Build & run

All commands run from the repo root unless noted.

- `make` — builds the UEFI bootloader via the `boot/` subdir Makefile.
- `make run` — builds then launches QEMU with OVMF firmware and a FAT disk made from `build/`. Requires `ovmf` installed (`sudo apt install ovmf`); the Makefile auto-detects OVMF code/vars files under `/usr/share/OVMF*`.
- `make clean` — wipes `build/`.
- `make lsp` — regenerates `compile_commands.json` using `bear` (falls back to `compiledb`).

**Toolchain** (from `boot/Makefile`): `clang` + `lld-link` with `--target=x86_64-unknown-windows`, `-ffreestanding -fno-stack-protector -fno-builtin -fno-pic -fno-pie -fshort-wchar -mno-red-zone -fms-extensions`. Link flags: `-entry:efi_main -subsystem:efi_application -dll`. There is no Makefile target for tests — none exist yet.

## Architecture

Two-layer layout:

- **`boot/`** — the UEFI bootloader. This is what the firmware loads.
  - `main_boot.c` — entry point `efi_main(EFI_HANDLE, EFI_SYSTEM_TABLE*)`. Currently just calls `ConOut->OutputString` and spins.
  - `boot/Makefile` — toolchain and QEMU invocation. The QEMU command line bakes in a FAT disk from `build/` and disables networking; change there if you need new devices.
- **`include/`** — currently empty; intended for kernel headers shared with the bootloader.
- **`boot/uefi/`** — hand-written subset of the UEFI spec types. **Not** the real EDK II / gnu-efi headers — only what's needed so far.
  - `types.h` — base UEFI typedefs (`UINTN`, `CHAR16`, `EFI_STATUS`, `EFI_TABLE_HEADER`, calling-convention macros, `EFIAPI = __attribute__((ms_abi))`). 
  - `system_table.h` — `EFI_SYSTEM_TABLE` layout with offset comments and a forward-declared `EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL` (only `Reset` and `OutputString` are wired up).
  - `boot_services.h` — partial `EFI_BOOT_SERVICES`: TPL, memory, events/timers, `ExitBootServices`. Protocol-handler and image-service entries are stubbed as `VOID *` (use `boot_services.h` extensions to type them when needed).

**Conventions**
- Code style: see `.clang-format` — 2-space indent, LLVM base, 100-col, `BreakBeforeBraces: Attach`, no short blocks on one line, separate definition blocks. Run `clang-format` on new files.
- Headers use `#pragma once` and include `types.h` first where they depend on base typedefs.
- Line endings: LF for `*.c *.h *.asm *.s *.S` (see `.gitattributes`).
- `compile_commands.json` is git-ignored.

**Known gaps to be aware of when extending**
- `EFI_GUID` is typed as `__int128`; real GUIDs (e.g. the `EFI_EVENT_GROUP_*` constants in `boot_services.h`) are 16-byte braced initializers, not parsed as GUIDs — fine for matching but not for any code that does field-wise access.
- Most `EFI_BOOT_SERVICES` protocol-handler functions are untyped `VOID *`; cast at the call site or extend the header before calling them.
- No `EFI_RUNTIME_SERVICES` table yet; no GOP, no file-system, no memory-map usage — needed before the bootloader can hand off to a kernel.
- No kernel or handoff code exists yet; `include/` is empty.
