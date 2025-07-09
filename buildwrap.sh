#!/bin/bash

if [ -d ".build_success" ]; then
    rm .build_success
fi

clear && ./build.sh --release --nostatic || exit 1
