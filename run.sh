#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export OPTIKEY_CONFIG_DIR="$project_dir/runtime/config/OptiKey/OptiKey-Pro"
export XDG_CACHE_HOME="$project_dir/runtime/cache"
mkdir -p "$OPTIKEY_CONFIG_DIR" "$XDG_CACHE_HOME"
exec "$project_dir/build/optikey-pro" "$@"
