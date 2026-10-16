#!/usr/bin/env bash
# Builds and runs every native test (tests/native/*_test.cpp). The tests need
# only a C++11 compiler, no Defold. Each test is linked with all sources in
# defold-pixels/src except pixels.cpp, which needs the Defold SDK.
#
#   tests/native/run.sh
#
# CXX picks the compiler (default: c++). Binaries go to build/native-tests.
# Exits non-zero when any test fails to build or fails a check.

set -u

root=$(cd "$(dirname "$0")/../.." && pwd)
src="$root/defold-pixels/src"
out="$root/build/native-tests"
cxx=${CXX:-c++}
mkdir -p "$out"

sources=()
for file in "$src"/*.cpp; do
    if [ "$(basename "$file")" != pixels.cpp ]; then
        sources+=("$file")
    fi
done

status=0
for test in "$root"/tests/native/*_test.cpp; do
    name=$(basename "$test" .cpp)
    echo "== $name"
    if ! "$cxx" -std=c++11 -O2 -Wall -Wextra -Werror -I "$src" "${sources[@]}" "$test" -o "$out/$name"; then
        echo "build failed: $name"
        status=1
        continue
    fi
    if ! "$out/$name"; then
        status=1
    fi
done
exit $status
