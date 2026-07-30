#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BASE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_ROOT="${BASE_DIR}/build"
VARS="${BUILD_ROOT}/my_vars.fd"

mkdir -p "${BUILD_ROOT}"

OVMF_DIR=""
for d in /usr/share/OVMF /usr/share/ovmf /usr/share/OVMF/x64 /usr/share/ovmf/x64; do
    if [ -d "$d" ]; then
        OVMF_DIR="$d"
        break
    fi
done

OVMF_CODE_PATH=""
OVMF_VARS_PATH=""

if [ -n "$OVMF_DIR" ]; then
    OVMF_CODE_PATH=$(find "$OVMF_DIR" -name "OVMF_CODE*.fd" 2>/dev/null | head -n 1)
    OVMF_VARS_PATH=$(find "$OVMF_DIR" -name "OVMF_VARS*.fd" 2>/dev/null | head -n 1)
fi

if [ -z "$OVMF_CODE_PATH" ] || [ -z "$OVMF_VARS_PATH" ]; then
    echo "=========================================================="
    echo " Error: Components OVMF UEFI not found on your system!!"
    echo " Please, install that:"
    echo "    sudo apt update && sudo apt install ovmf"
    echo "=========================================================="
    exit 1
fi

if [ ! -f "$VARS" ]; then
    cp "$OVMF_VARS_PATH" "$VARS"
    chmod +w "$VARS"
fi

# Start QEMU
qemu-system-x86_64 \
    -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE_PATH" \
    -drive if=pflash,format=raw,file="$VARS" \
    -drive file=fat:rw:"$BUILD_ROOT",format=raw \
    -serial stdio \
    -d int,cpu_reset,guest_errors -no-reboot -no-shutdown \
    -enable-kvm -cpu host \
    -D qemu.log \
    -machine q35 \
    -net none
