#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export GAZE_GRID_CONFIG_DIR="$project_dir/runtime/config/gaze-grid/gaze-grid"
export XDG_CACHE_HOME="$project_dir/runtime/cache"
mkdir -p "$GAZE_GRID_CONFIG_DIR" "$XDG_CACHE_HOME"
exec "$project_dir/build/gaze-grid" "$@"
