# Activate the Zephyr environment for P02. Source it in every new terminal before using west:
#
#   source tools/zephyr_env.sh
#
# It activates the Python environment that tools/zephyr_setup.sh created and points Zephyr at the
# release pinned in west.yml (deps/zephyr). Do not mix it with ESP-IDF's export.sh or the Zephyr
# course's env.sh in the same terminal.

if [ -n "${ZSH_VERSION:-}" ]; then
    P02_ROOT="$(cd "$(dirname "${(%):-%x}")/.." && pwd)"
else
    P02_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
fi
# Exported so that commands (e.g. -DBOARD_ROOT=$P02_ROOT/zephyr) and other scripts can find the
# repository without a hard-coded path.
export P02_ROOT

if [ ! -f "$P02_ROOT/.venv/bin/activate" ] || [ ! -d "$P02_ROOT/deps/zephyr" ]; then
    echo "error: the Zephyr workspace is not set up. Run tools/zephyr_setup.sh first." >&2
    return 1 2>/dev/null || exit 1
fi

source "$P02_ROOT/.venv/bin/activate"
export ZEPHYR_BASE="$P02_ROOT/deps/zephyr"

# The Zephyr course's env.sh sets this to "host" (the PC's own GCC), which breaks ESP32-S3 builds.
unset ZEPHYR_TOOLCHAIN_VARIANT
