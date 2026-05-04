#!/usr/bin/env bash

set -euo pipefail

LOG_FILE="${LOG_FILE:-server.log}"

usage() {
  echo "Usage:"
  echo "  $0 start <server-executable> <port>"
  echo "  $0 status <pid>"
  echo "  $0 stop <pid>"
  exit 1
}

if [[ "$#" -lt 2 ]]; then
  usage
fi

cmd="$1"

case "$cmd" in
  start)
    if [[ "$#" -ne 3 ]]; then
      usage
    fi
    exe="$2"
    port="$3"

    if [[ ! -f "$exe" ]]; then
      echo "$exe executable doesn't exist!" >&2
      exit 1
    fi
    if [[ ! -x "$exe" ]]; then
      echo "$exe is not executable!" >&2
      exit 1
    fi

    nohup "$exe" "$port" >>"$LOG_FILE" 2>&1 &
    pid=$!
    disown "$pid" 2>/dev/null || true

    sleep 0.2
    if kill -0 "$pid" 2>/dev/null; then
      echo "[$(date '+%Y-%m-%d %H:%M:%S')] started $exe on port $port (pid=$pid)" | tee -a "$LOG_FILE"
      echo "$pid"
    else
      echo "[$(date '+%Y-%m-%d %H:%M:%S')] failed to start $exe on port $port" | tee -a "$LOG_FILE" >&2
      exit 1
    fi
    ;;

  status)
    pid="$2"
    if ps -p "$pid" -o pid=,stat=,etime=,command= 2>/dev/null; then
      :
    else
      echo "no process with pid $pid"
      exit 1
    fi
    ;;

  stop)
    pid="$2"
    if ! kill -0 "$pid" 2>/dev/null; then
      echo "no process with pid $pid"
      exit 1
    fi

    kill -INT "$pid" 2>/dev/null || true

    for _ in 1 2 3 4 5 6 7 8 9 10; do
      if ! kill -0 "$pid" 2>/dev/null; then
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] stopped pid=$pid (SIGINT)" | tee -a "$LOG_FILE"
        exit 0
      fi
      sleep 0.5
    done

    kill -TERM "$pid" 2>/dev/null || true
    for _ in 1 2 3 4 5 6 7 8 9 10; do
      if ! kill -0 "$pid" 2>/dev/null; then
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] stopped pid=$pid (SIGTERM)" | tee -a "$LOG_FILE"
        exit 0
      fi
      sleep 0.5
    done

    echo "pid $pid did not exit after SIGINT/SIGTERM" >&2
    exit 1
    ;;

  *)
    usage
    ;;
esac
