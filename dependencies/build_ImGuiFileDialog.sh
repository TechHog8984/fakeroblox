#!/bin/bash

fail () {
    echo "error: $1"
    exit 1
}

if [ ! -d "./rlImGui/imgui-master" ]; then
    fail "run ./build_rlImGui.sh first (or ensure ./rlImGui/imgui-master)"
fi

pushd ImGuiFileDialog || fail "pushd ImGuiFileDialog failed"

perl -pi -e 's|target_include_directories\(ImGuiFileDialog PUBLIC \${CMAKE_CURRENT_SOURCE_DIR}\)|target_include_directories(ImGuiFileDialog PUBLIC\n \${CMAKE_CURRENT_SOURCE_DIR}\n \${CMAKE_CURRENT_SOURCE_DIR}/../rlImGui/imgui-master\n)|g' CMakeLists.txt

if [ ! -d "cmake" ]; then
    mkdir cmake || exit 1
fi

pushd cmake || fail "pushd cmake failed"

echo "[build] running initial cmake"
cmake .. || exit 1

echo "[build] building"
make || exit 1

echo "[build] finished"

popd || fail "popd cmake failed"

popd || fail "popd ImGuiFileDialog failed"
