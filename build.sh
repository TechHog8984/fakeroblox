#!/bin/bash

LUAU_SOURCES="dependencies/Luau/Analysis/src/*.cpp dependencies/Luau/Ast/src/*.cpp dependencies/Luau/Compiler/src/*.cpp dependencies/Luau/Config/src/*.cpp dependencies/Luau/VM/src/*.cpp"
LUAU_INCLUDE="-Idependencies/Luau/Analysis/include -Idependencies/Luau/Ast/include -Idependencies/Luau/Common/include -Idependencies/Luau/Compiler/include -Idependencies/Luau/Config/include -Idependencies/Luau/VM/src -Idependencies/Luau/VM/include"

LUAU_SOURCES_BUILD=$(echo $LUAU_SOURCES | sed 's/dependencies\//..\/..\/..\/dependencies\//g' -)
LUAU_INCLUDE_BUILD=$(echo $LUAU_INCLUDE | sed 's/dependencies\//..\/..\/..\/dependencies\//g' -)

PLATFORM=linux
RELEASE_FLAGS=
STATIC_FLAGS=-static
ASAN_FLAGS=

while [[ $# -gt 0 ]]; do
    case $1 in
        linux)
            PLATFORM=linux
            shift
            ;;
        windows)
            PLATFORM=windows
            shift
            ;;
        --release)
            RELEASE_FLAGS=-O2
            shift
            ;;
        --nostatic)
            STATIC_FLAGS=
            shift
            ;;
        --asan)
            ASAN_FLAGS=-fsanitize=address
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

outname=fakeroblox

outfile=build/$outname

if [[ "$PLATFORM" == "linux" ]]; then
    compiler=g++
    builddir=build/linux
elif [[ "$PLATFORM" == "windows" ]]; then
    compiler=x86_64-w64-mingw32-g++-posix
    if ! command -v $compiler >/dev/null 2>&1; then
        compiler="x86_64-w64-mingw32-g++"
    fi
    outfile=$outfile.exe
    builddir=build/windows
else
    echo "unknown platform"
    exit 1
fi

if [ ! -d "build" ]; then
    mkdir build
fi

if [ ! -d "$builddir" ]; then
    mkdir $builddir
fi

luaubuilddir=$builddir/Luau
if [ ! -d "$luaubuilddir" ]; then
    mkdir $luaubuilddir
    cd $luaubuilddir
    echo "building luau..."
    $compiler -std=c++17 -g -O2 -c $LUAU_SOURCES_BUILD $LUAU_INCLUDE_BUILD || exit 1
    ar rcs libluau.a *.o || exit 1
    echo "luau built!"
    cd ../../..
fi

rlimguibuilddir=$builddir/rlImGui
if [ ! -d "$rlimguibuilddir" ]; then
    if [[ "$PLATFORM" != "linux" ]]; then
        echo "TODO: verify rlImGui builds on other platforms; premake can take a compiler!"
        exit 1
    fi
    mkdir $rlimguibuilddir
    pushd dependencies/rlImGui
    echo "building rlImGui..."
    sed -i -e 's/"IMGUI_DISABLE_OBSOLETE_FUNCTIONS",//g' premake5.lua
    chmod +x ./premake5 || exit 1
    ./premake5 gmake || exit 1
    make config=release_x64 || exit 1
    ar rcs librlimgui.a ./build/obj/x64/Release/rlImGui/*.o || exit 1
    echo "rlImGui built!"
    popd
    mv dependencies/rlImGui/librlimgui.a $rlimguibuilddir
fi

# texteditorbuilddir=$builddir/ImGuiColorTextEdit
# if [ ! -d "$texteditorbuilddir" ]; then
#     mkdir $texteditorbuilddir
#     pushd $texteditorbuilddir
#     echo "building ImGuiColorTextEdit..."
#     $compiler -std=c++17 -O2 -c ../../../dependencies/ImGuiColorTextEdit/*.cpp ../../../dependencies/rlImGui/build/obj/x64/Release/rlImGui/imgui.* -I../../../dependencies/rlImGui/imgui-master || exit 1
#     ar rcs libgimguicolortextedit.a *.o || exit 1
#     echo "ImGuiColorTextEdit built!"
#     popd
# fi

fakerobloxbuilddir=$builddir/fakeroblox
if [ -d $fakerobloxbuilddir ]; then
    rm -r $fakerobloxbuilddir
fi
mkdir $fakerobloxbuilddir

echo "building fakeroblox..."
pushd $fakerobloxbuilddir
$compiler -std=c++17 -g -Wall $RELEASE_FLAGS $ASAN_FLAGS -c ../../../src/*.cpp ../../../src/*/*.cpp ../../../src/*/*/*.cpp ../../../src/*/*/*/*.cpp -I../../../include $LUAU_INCLUDE_BUILD -lcurl -L../Luau -lluau \
    -I../../../dependencies/rlImGui/imgui-master || exit 1
ar rcs libfakeroblox.a *.o || exit 1
popd
echo "fakeroblox built"

echo "buildling cli..."
$compiler -std=c++17 -g -Wall $RELEASE_FLAGS $ASAN_FLAGS $STATIC_FLAGS -o $outfile main.cpp -Iinclude $LUAU_INCLUDE \
    -Idependencies/rlImGui -Idependencies/rlImGui/imgui-master -Idependencies/ImGuiColorTextEdit -lcurl -lraylib \
    -L$fakerobloxbuilddir -lfakeroblox -L$luaubuilddir -lluau -L$rlimguibuilddir -lrlimgui || exit 1
echo "cli built to $outfile"
