import shutil
import subprocess
import sys
import tomllib
from pathlib import Path

# General paths project
BASE_DIR = Path(__file__).parent.parent.resolve()
CONFIG_PATH = BASE_DIR / "nova_build.toml"
BUILD_DIR = BASE_DIR / "build"
NINJA_FILE = BASE_DIR / "build.ninja"


def load_config() -> dict:
    if not CONFIG_PATH.exists():
        print(f"Error: Configuration file '{CONFIG_PATH}' not found!")
        sys.exit(1)

    with open(CONFIG_PATH, "rb") as f:
        return tomllib.load(f)


def _is_msvc_linker(linker: str) -> bool:
    """True for lld-link / link.exe (MSVC-style drivers); False for GNU ld/lld."""
    base = Path(linker).name.lower()
    return base in ("lld-link", "lld-link.exe", "link", "link.exe")


def resolve_context(val, context: dict):
    """ Change ${var} on value from context (for example:
                                             ${arch} -> x86_64) """
    if isinstance(val, str):
        for k, v in context.items():
            # Change from text to value
            val = val.replace(f"${{{k}}}", str(v))
        return val
    elif isinstance(val, list):
        return [resolve_context(item, context) for item in val]
    return val


def find_target_sources(target_cfg: dict, context: dict) -> list[Path]:
    """ Find files C due to 'dirs' + 'exclude' or due'sources' """
    sources = []

    # 1. Find directories
    dirs = resolve_context(target_cfg.get("dirs", []), context)
    exclude_patterns = resolve_context(target_cfg.get("exclude", []), context)

    for d in dirs:
        dir_path = BASE_DIR / d
        if dir_path.exists():
            sources.extend(dir_path.rglob("*.c"))

    # 2. Find sources
    raw_sources = resolve_context(target_cfg.get("sources", []), context)
    for src_pattern in raw_sources:
        if "*" in src_pattern or "?" in src_pattern:
            sources.extend(BASE_DIR.glob(src_pattern))
        else:
            p = BASE_DIR / src_pattern
            if p.exists():
                sources.append(p)

    # 3. Filter by exclude patterns (substring match against any path part)
    if exclude_patterns:
        sources = [
            src for src in sources
            if not any(ex in src.parts for ex in exclude_patterns)
        ]

    # Delete duplicats
    return sorted(list(set(sources)))


def generate_ninja():
    config = load_config()

    project_cfg = config.get("project", {})
    toolchain_cfg = config.get("toolchain", {})
    context = project_cfg.copy()

    context.setdefault("arch", "x86_64")
    context.setdefault("name", "NovaOS")

    default_cc = toolchain_cfg.get("cc", "clang")
    default_linker = toolchain_cfg.get("linker", "ld.lld")
    common_cflags = resolve_context(toolchain_cfg.get("common_cflags", []),
                                    context)

    ninja_lines = [
        "# Generated automatically by NovaBuild",
        "ninja_required_version = 1.3\n",
        "# Global compilation rules",
        "rule cc",
        "  command = $cc $cflags -c $in -o $out",
        "  description = CC $in\n",
        "rule link_gnu",
        "  command = $linker $ldflags $in -o $out",
        "  description = LINK $out\n",
        "rule link_msvc",
        "  command = $linker $ldflags $in -out:$out",
        "  description = LINK $out\n",
    ]

    all_outputs = []

    targets = config.get("targets", {})
    if not targets:
        reserved = {"project", "toolchain", "profiles"}
        targets = {k: v for k, v in config.items() if k not in reserved}

    for target_name, target_cfg in targets.items():
        cc = resolve_context(target_cfg.get("cc", default_cc), context)
        linker = resolve_context(target_cfg.get("linker", default_linker),
                                 context)
        local_cflags = resolve_context(target_cfg.get("cflags", []), context)
        cflags_str = " ".join(common_cflags + local_cflags)
        ldflags_str = " ".join(resolve_context(target_cfg.get("ldflags", []),
                                               context))

        resolved_sources = find_target_sources(target_cfg, context)
        if not resolved_sources:
            print(f"Warning: No sources found for target '{target_name}'")
            continue

        objects = []
        for src in resolved_sources:
            rel_src = src.relative_to(BASE_DIR)
            obj = BUILD_DIR / "obj" / rel_src.with_suffix(".o")
            obj.parent.mkdir(parents=True, exist_ok=True)
            obj_str = obj.as_posix()
            in_str = rel_src.as_posix()
            objects.append(obj_str)

            ninja_lines.append(f"build {obj_str}: cc {in_str}")
            ninja_lines.append(f"  cc = {cc}")
            ninja_lines.append(f"  cflags = {cflags_str}")

        target_type = target_cfg.get("type", "")
        if target_type in ["uefi", "uefi_app"]:
            out_bin = BUILD_DIR / "EFI" / "BOOT" / "BOOTX64.EFI"
        else:
            out_bin = BUILD_DIR / f"{target_name}"
        out_bin.parent.mkdir(parents=True, exist_ok=True)
        out_bin_str = out_bin.as_posix()

        all_outputs.append(out_bin_str)

        is_msvc_linker = _is_msvc_linker(linker)
        rule_name = "link_msvc" if is_msvc_linker else "link_gnu"

        objs_str = " ".join(objects)
        ninja_lines.append(f"build {out_bin_str}: {rule_name} {objs_str}")
        ninja_lines.append(f"  linker = {linker}")
        ninja_lines.append(f"  ldflags = {ldflags_str}\n")

    if all_outputs:
        ninja_lines.append(f"default {' '.join(all_outputs)}")

    with open(NINJA_FILE, "w", encoding="utf-8") as f:
        f.write("\n".join(ninja_lines) + "\n")

    print(f"Generated {NINJA_FILE.name}")


def generate_compile_commands():
    res = subprocess.run(
        ["ninja", "-t", "compdb", "cc"],
        cwd=BASE_DIR,
        capture_output=True,
        text=True,
    )
    if res.returncode == 0:
        compdb_path = BASE_DIR / "compile_commands.json"
        with open(compdb_path, "w", encoding="utf-8") as f:
            f.write(res.stdout)
        print("Generated compile_commands.json for LSP/clangd")


def build():
    generate_ninja()
    print("\nStarting Ninja...")
    res = subprocess.run(["ninja"], cwd=BASE_DIR)

    if res.returncode == 0:
        generate_compile_commands()
        print("\nBuild success!")
    else:
        print("\nBuild failed!")


def clean():
    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
        print("Directory 'build' cleaned.")

    if NINJA_FILE.exists():
        NINJA_FILE.unlink()
        print("File 'build.ninja' cleaned.")

    compdb_path = BASE_DIR / "compile_commands.json"
    if compdb_path.exists():
        compdb_path.unlink()
        print("File 'compile_commands.json' cleaned.")


def main():
    print("=== NovaBuild Started ===")
    if len(sys.argv) > 1 and sys.argv[1] == "clean":
        clean()
    else:
        build()


if __name__ == "__main__":
    main()
