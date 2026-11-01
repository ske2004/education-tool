#!/bin/sh
# macOS (and many Linux distros) only ship python3, not python.
if command -v python3 >/dev/null 2>&1; then
  exec python3 build.py "$@"
else
  exec python build.py "$@"
fi
