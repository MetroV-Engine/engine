This folder contains the source code of the engine.

It is where the modules of `src/` are connected: `main.cpp` builds an ECS
world (`src/ECS`) and opens the editor (`src/Editor`) on it.

From this folder (it adds the ECS and the editor itself):

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --target engine_gui
./build/engine_gui
```

The toolchain is only needed on the first configure, and not at all when the
engine's `vcpkg` submodule is initialized or `VCPKG_ROOT` is set.

From the engine root:

```sh
cmake -S . -B build -DENGINE_BUILD_GUI=ON
cmake --build build --target engine_gui
./build/engine_gui
```
