#!/bin/bash

if [ -d ".build_success" ]; then
    rm .build_success
fi

# clear && ./build.sh --release --nostatic || exit 1
clear && ./mate || exit 1
