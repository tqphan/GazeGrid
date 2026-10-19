#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export XDG_CONFIG_HOME="$project_dir/runtime/config"
export XDG_CACHE_HOME="$project_dir/runtime/cache"
mkdir -p "$XDG_CONFIG_HOME" "$XDG_CACHE_HOME"
exec "$project_dir/build/optikey-pro" "$@"
