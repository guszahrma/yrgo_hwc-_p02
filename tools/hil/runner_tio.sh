#!/usr/bin/env bash
# Opens tio on a board that has a self-hosted runner, with the runner paused: waits for a running job
# to finish, stops the runner, runs tio, and starts the runner again when tio exits.
#
#   tools/hil/runner_tio.sh <board> [tio options]     e.g. tools/hil/runner_tio.sh daisy
#
# <board> is nano, daisy or nucleo, the runner folder's name (arduino_nano_esp32, ...) or the port.
# Board, port, folder and service come from the runner guard's file (/etc/p02-runner-guard.conf).
# The pause is a file with this script's PID that the guard reads: while the PID lives, the guard
# leaves the runner stopped; if the script dies without cleaning up, the guard starts the runner again
# within 30 s.
set -u

CONFIG=${CONFIG:-/etc/p02-runner-guard.conf}
STATE_DIR=/run/p02-runner-guard
POLL_S=5

# Short name of a runner folder's board
short_name() {
    case "$(basename "$1")" in
        arduino_nano_esp32) echo nano ;;
        daisy_seed) echo daisy ;;
        nucleo_g0b1re) echo nucleo ;;
        *) basename "$1" ;;
    esac
}

usage() {
    cat >&2 << EOF
usage: $0 <board or port> [tio options]

Boards in $CONFIG:
EOF
    while read -r p d s _; do
        [[ -z "${p:-}" || "$p" == \#* ]] && continue
        local attached=detached runner=stopped
        [[ -e "$p" ]] && attached=attached
        systemctl is-active --quiet "$s" && runner=running
        pgrep -f "^$d/bin/Runner.Worker" > /dev/null && runner="running, job in progress"
        printf '  %-8s %-28s board %-8s runner %s\n' "$(short_name "$d")" "$p" "$attached" "$runner" >&2
    done < "$CONFIG"
    cat >&2 << EOF

Quit tio with Ctrl+T, q; the runner starts again.
EOF
}

if [[ $# -lt 1 || "$1" == -h || "$1" == --help ]]; then
    usage
    exit 2
fi
board=$1
shift

port='' dir='' service=''
while read -r p d s _; do
    [[ -z "${p:-}" || "$p" == \#* ]] && continue
    if [[ "$board" == "$(short_name "$d")" || "$board" == "$(basename "$d")" || "$board" == "$p" ]]; then
        port=$p dir=$d service=$s
        break
    fi
done < "$CONFIG"
if [[ -z "$service" ]]; then
    echo "error: no runner for '$board'" >&2
    echo >&2
    usage
    exit 1
fi

paused="$STATE_DIR/$(basename "$dir").paused"
was_active=false
systemctl is-active --quiet "$service" && was_active=true

sudo -v || exit 1

resume() {
    trap - EXIT INT TERM HUP
    # -n: no password prompt at the end; if sudo has expired, the guard resumes the runner
    if sudo -n rm -f "$paused" 2>/dev/null; then
        if $was_active && [[ -e "$port" ]]; then
            sudo -n systemctl start "$service" && echo "Runner started again: $service"
        fi
    else
        echo "Could not resume the runner without a password: the guard starts it within 30 s."
    fi
}
trap resume EXIT
trap 'exit 130' INT TERM HUP

sudo mkdir -p "$STATE_DIR"
echo $$ | sudo tee "$paused" > /dev/null

if $was_active; then
    # A running job has a Runner.Worker process started from the runner's folder (as in the guard)
    if pgrep -f "^$dir/bin/Runner.Worker" > /dev/null; then
        echo "A job is running on the $(short_name "$dir") runner: waiting for it to finish (Ctrl+C to give up)..."
        while pgrep -f "^$dir/bin/Runner.Worker" > /dev/null; do
            sleep "$POLL_S"
        done
    fi
    sudo systemctl stop "$service"
    echo "Runner paused: $service"
fi

tio "$@" "$port"
