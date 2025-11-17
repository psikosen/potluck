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

## Testing

Tests exercise the database schema initialization and filesystem scanning helpers:

```
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Tasks

See [`task.md`](task.md) for the current checklist.
