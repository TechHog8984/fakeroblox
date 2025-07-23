#!/bin/bash

fail () {
    echo "error: $1"
    exit 1
}

pushd Luau || fail "pushd Luau failed"

if [ ! -d "cmake" ]; then
    mkdir cmake || exit 1
fi

pushd cmake || fail "pushd cmake failed"

echo "[build] running initial cmake"
cmake .. -DCMAKE_BUILD_TYPE=Release || exit 1

echo "[build] building"
cmake --build . --target Luau.Analysis --config Release
cmake --build . --target Luau.Ast --config Release
cmake --build . --target Luau.Compiler --config Release
cmake --build . --target Luau.Config --config Release
cmake --build . --target Luau.VM --config Release

echo "[build] finished building"

echo "[build] merging archives"
ar -x ./libLuau.Analysis.a || exit 1
ar -x ./libLuau.Ast.a || exit 1
ar -x ./libLuau.Compiler.a || exit 1
ar -x ./libLuau.Config.a || exit 1
ar -x ./libLuau.VM.a || exit 1
ar rcs libLuau.a ./*.o || exit 1

echo "[build] finished"

popd || fail "popd cmake failed"

popd || fail "popd luau failed"
