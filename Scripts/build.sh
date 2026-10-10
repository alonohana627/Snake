#!/usr/bin/env bash
# Configure and build Snake with CMake + Ninja.
#
# Usage: Scripts/build.sh [debug|release] [--tests] [--clean]
set -euo pipefail

config="debug"
tests=0
clean=0

for arg in "$@"; do
    case "$arg" in
        debug|release) config="$arg" ;;
        --tests)       tests=1 ;;
        --clean)       clean=1 ;;
        -h|--help)
            echo "Usage: $0 [debug|release] [--tests] [--clean]"
            exit 0
            ;;
        *)
            echo "Unknown argument: $arg" >&2
            exit 1
            ;;
    esac
done

preset="$config"
if [[ $tests -eq 1 ]]; then
    preset="$config-tests"
fi

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$root"

if [[ $clean -eq 1 ]]; then
    rm -rf "build/$preset"
fi

cmake --preset "$preset"
cmake --build --preset "$preset"

if [[ $tests -eq 1 ]]; then
    ctest --preset "$preset"
fi
