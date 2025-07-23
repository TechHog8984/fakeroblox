# fakeroblox

fakeroblox is an attempt to create a Luau environment that is as close to Roblox's as possible.

# BUILDING
fakeroblox uses [mate.h](https://github.com/TomasBorquez/mate.h/) for its core build system, along with some shell files (sorry) to build the dependencies

to build the dependencies, just run the two scripts inside the dependencies folder with bash:
```bash
cd dependencies

bash ./build_luau.sh
bash ./build_rlImGui.sh

cd ..
```

then, to bootstrap mate, compile and run mate.c (note that I target gcc, so clang and msvc may or may not be supported):
```bash
gcc -o mate mate.c
./mate
```

that's it! to build again, simple run `./mate` just like before and it will detect any changes made and recompile only what's needed

# GOALS
* Instance system
* 1:1 Roblox error messages
* 2d graphics
* exploit functions (most similar to Synapse)

# NONGOALS
* 3d graphics
* networking
* rbxl or rbxm parsing

Contributions to nongoals may be welcome, but they aren't something I will be focusing on.
