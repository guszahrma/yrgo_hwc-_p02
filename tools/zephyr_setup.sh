#!/usr/bin/env bash
#
# Set up the Zephyr workspace for P02, inside the repository:
#
#   .venv/          Python environment with west and Zephyr's Python tools.
#   .west/config    Makes the repository the west workspace, with west.yml as its manifest.
#   deps/           The Zephyr release and modules that west.yml pins.
#
# The Zephyr SDK (compiler) is installed once per user in ~/zephyr-sdk-<version>, and only if no
# ESP32-S3 toolchain is found there already.
#
# Running the script again is safe: it brings the workspace up to date with west.yml.
#
# Usage (from anywhere):
#   tools/zephyr_setup.sh
# Then, in every new terminal:
#   source tools/zephyr_env.sh
set -euo pipefail

# Absolute path to the repository root.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# Toolchain for the Arduino Nano ESP32 (ESP32-S3, Xtensa).
TOOLCHAIN="xtensa-espressif_esp32s3_zephyr-elf"

################################################################################
# Terminate the script if a host tool the workspace needs is missing.
################################################################################
check_host_tools() {
    local missing=()
    for tool in git cmake python3; do
        command -v "$tool" &> /dev/null || missing+=("$tool")
    done
    if ! python3 -c "import venv, ensurepip" &> /dev/null; then
        missing+=("python3-venv")
    fi
    if [ ${#missing[@]} -ne 0 ]; then
        echo "error: missing ${missing[*]}. See 'Packages in Ubuntu' in readme.md." >&2
        exit 1
    fi
}

check_host_tools

# The Python environment, with west and ninja.
if [ ! -x .venv/bin/python3 ]; then
    echo "==== Creating the Python environment in .venv"
    python3 -m venv .venv
fi
.venv/bin/pip install --quiet --upgrade pip
.venv/bin/pip install --quiet west ninja

# Make the repository the west workspace. `west init -l` would put the workspace in the parent
# directory (the home directory) instead, so the configuration is written directly.
if [ ! -f .west/config ]; then
    echo "==== Creating the west workspace"
    mkdir -p .west
    printf '[manifest]\npath = .\nfile = west.yml\n' > .west/config
fi
.venv/bin/west config zephyr.base deps/zephyr

echo "==== Fetching Zephyr and its modules (the first time takes a few minutes)"
.venv/bin/west update --narrow -o=--depth=1

echo "==== Installing Zephyr's Python requirements"
ZEPHYR_BASE="$ROOT/deps/zephyr" .venv/bin/west packages pip --install

if compgen -G "$HOME/zephyr-sdk-*/gnu/$TOOLCHAIN" > /dev/null; then
    echo "==== Zephyr SDK with $TOOLCHAIN already installed"
else
    echo "==== Installing the Zephyr SDK ($TOOLCHAIN)"
    (cd deps/zephyr && ZEPHYR_BASE="$ROOT/deps/zephyr" "$ROOT/.venv/bin/west" sdk install -t "$TOOLCHAIN")
fi

echo "==== Fetching the Espressif binary blobs"
ZEPHYR_BASE="$ROOT/deps/zephyr" .venv/bin/west blobs fetch hal_espressif

echo
echo "Setup complete. In every new terminal, run 'source tools/zephyr_env.sh' before using west."
