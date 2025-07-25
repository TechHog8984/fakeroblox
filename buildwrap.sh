#!/bin/bash

if [ -f ".test_success" ]; then
    rm .test_success
fi

# clear && ./build.sh --release --nostatic || exit 1
clear && ./mate || exit 1
