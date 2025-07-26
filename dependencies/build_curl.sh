#!/bin/bash

fail () {
    echo "error: $1"
    exit 1
}

pushd curl || fail "pushd curl failed"

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

popd || fail "popd curl failed"
