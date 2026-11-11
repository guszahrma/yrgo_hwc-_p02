#!/usr/bin/env bash
# Keeps a board's self-hosted runner online only while its board is attached: starts the runner's
# service when the port appears, stops it when the port has been gone for GRACE_S seconds and no job
# runs. Run as root by p02-runner-guard.timer; reads "<port> <runner folder>" lines from CONFIG.
# The grace time and the job check are there because a board leaves USB for a moment each time a job
# flashes and resets it.
set -u

CONFIG=${CONFIG:-/etc/p02-runner-guard.conf}
GRACE_S=${GRACE_S:-120}
STATE_DIR=/run/p02-runner-guard

mkdir -p "$STATE_DIR"
now=$(date +%s)

while read -r port dir _; do
    [[ -z "${port:-}" || "$port" == \#* ]] && continue
    if [[ ! -r "$dir/.service" ]]; then
        echo "$dir: no .service file (runner not installed with svc.sh?)" >&2
        continue
    fi
    service=$(<"$dir/.service")
    missing="$STATE_DIR/$(basename "$dir").missing_since"

    if [[ -e "$port" ]]; then
        rm -f "$missing"
        if ! systemctl is-active --quiet "$service"; then
            echo "$port attached: starting $service"
            systemctl start "$service"
        fi
        continue
    fi

    systemctl is-active --quiet "$service" || continue
    [[ -f "$missing" ]] || echo "$now" > "$missing"
    (( now - $(<"$missing") >= GRACE_S )) || continue
    # A running job has a Runner.Worker process started from the runner's folder
    if pgrep -f "^$dir/bin/Runner.Worker" > /dev/null; then
        continue
    fi
    echo "$port gone for ${GRACE_S} s: stopping $service"
    systemctl stop "$service"
    rm -f "$missing"
done < "$CONFIG"
