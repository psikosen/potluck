# GridFire v2.0

GridFire is an intelligence-driven filesystem explorer that tracks activity inside a SQLite database, scores files by heat, and prepares the ground for Dear ImGui rendering.

## Project layout

```
├── CMakeLists.txt
├── src/
│   ├── app.*
│   ├── analysis/
│   ├── assets/
│   ├── command/
│   ├── database/
│   ├── favorites/
│   ├── filesystem/
│   ├── git/
│   └── ui/
├── tests/
├── external/
│   ├── imgui/
│   └── stb/
└── task.md
```

## Building

The repository uses CMake. The UI stack depends on SDL2, OpenGL, and Dear ImGui sources. Enable it only after adding the external dependencies:

```
cmake -S . -B build -DGRIDFIRE_BUILD_APPLICATION=ON
cmake --build build
```

For core development (no SDL2/ImGui requirement):

```
cmake -S . -B build
cmake --build build
```

## Dependencies

GridFire relies on a short list of well-known libraries. The core build is tested with GCC 13.3 and SQLite 3.45.1.

- **SQLite3** – persistent metadata store (required, linked dynamically).
- **Threads/Pthreads** – concurrency primitives requested through `find_package(Threads)`.
- **Dear ImGui** – immediate mode UI toolkit (sources tracked under `external/imgui`).
- **stb_image** – lightweight image loader used by the UI (tracked under `external/stb`).
- **SDL2** and **OpenGL** – only required when `GRIDFIRE_BUILD_APPLICATION=ON` to provide rendering + platform integration.

Each dependency is tied to the versions bundled in the repository (ImGui/stb) or resolved from the active toolchain (SQLite3, SDL2, OpenGL). When upgrading, ensure the code paths in `CMakeLists.txt` keep matching the resolved versions.

## Testing

Tests exercise the database schema initialization and filesystem scanning helpers:

```
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Tasks

See [`task.md`](task.md) for the current checklist.
