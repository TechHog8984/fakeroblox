#!/bin/bash

fail () {
    echo "error: $1"
    exit 1
}

pushd rlImGui || fail "pushd rlImGui failed"

sed -i -e 's/"IMGUI_DISABLE_OBSOLETE_FUNCTIONS",//g' premake5.lua
chmod +x ./premake5 || exit 1
./premake5 gmake || fail "premake failed"
make config=release_x64 || fail "make failed"

echo "[build] finished"

popd || fail "popd rlImGui failed"
