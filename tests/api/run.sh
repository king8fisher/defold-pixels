#!/usr/bin/env bash
# Builds the engine tests (tests/api) with bob and runs them in a headless
# engine. The engine is built by the Defold build server, so this needs
# network access.
#
#   tests/api/run.sh
#
# BOB is the path to bob.jar or to the editor's defold-<sha>.jar. On macOS it
# defaults to the jar inside /Applications/Defold.app.
# JAVA picks the Java binary. It defaults to the editor's bundled JDK when BOB
# defaults to the editor, and to java otherwise.
# BUILD_SERVER defaults to https://build.defold.com.
# Exits with 0 when every check passes and 1 otherwise. The engine log is
# kept in build/api-tests/engine.log.

set -euo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)
# bob needs the output directory relative to the project root.
cd "$root"
out=build/api-tests
server=${BUILD_SERVER:-https://build.defold.com}

case "$(uname -s)-$(uname -m)" in
    Darwin-arm64) platform=arm64-macos; engine_dir=arm64-osx ;;
    Darwin-x86_64) platform=x86_64-macos; engine_dir=x86_64-osx ;;
    Linux-x86_64) platform=x86_64-linux; engine_dir=x86_64-linux ;;
    Linux-aarch64) platform=arm64-linux; engine_dir=arm64-linux ;;
    *) echo "unsupported host: $(uname -s) $(uname -m)" >&2; exit 2 ;;
esac

bob=${BOB:-}
java=${JAVA:-java}
if [ -z "$bob" ]; then
    resources=/Applications/Defold.app/Contents/Resources
    if [ ! -f "$resources/config" ]; then
        echo "set BOB to the path of bob.jar" >&2
        exit 2
    fi
    sha=$(sed -n 's/^editor_sha1 = //p' "$resources/config")
    jdk=$(sed -n 's|^jdk = .*/packages/||p' "$resources/config")
    bob="$resources/packages/defold-$sha.jar"
    java=${JAVA:-"$resources/packages/$jdk/bin/java"}
fi

"$java" -cp "$bob" com.dynamo.bob.Bob --root "$root" --platform "$platform" --variant headless \
    --settings "$root/tests/api/api.settings" --build-server "$server" --output "$out/default" build

engine="$root/build/$engine_dir/dmengine"
chmod +x "$engine"
log="$out/engine.log"
"$engine" "$out/default/game.projectc" > "$log" 2>&1 &
pid=$!

# The test script prints one summary line ("... api checks passed" or
# "... api checks failed") and then exits the engine. The engine can hang
# while shutting down (seen on macOS in dmGraphics::MetalDeleteContext), so
# the log decides the result, and the engine is stopped if it does not exit.
deadline=$((SECONDS + 60))
until grep -q "api checks" "$log" || ! kill -0 "$pid" 2>/dev/null || [ "$SECONDS" -ge "$deadline" ]; do
    sleep 0.2
done
grace=$((SECONDS + 5))
while kill -0 "$pid" 2>/dev/null && [ "$SECONDS" -lt "$grace" ]; do
    sleep 0.2
done
if kill -0 "$pid" 2>/dev/null; then
    kill "$pid"
fi
wait "$pid" 2>/dev/null || true
cat "$log"

if grep -q "SCRIPT: all .* api checks passed" "$log" && ! grep -q "SCRIPT: FAIL " "$log"; then
    exit 0
fi
echo "engine tests failed" >&2
exit 1
