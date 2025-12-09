cmake_minimum_required(VERSION 3.20)
project(GridFire VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GRIDFIRE_BUILD_APPLICATION "Build the SDL2/ImGui based UI" OFF)

find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

set(GRIDFIRE_CORE_SOURCES
    src/app.cpp
    src/database/sqlite_manager.cpp
    src/filesystem/filesystem_engine.cpp
    src/filesystem/stats_engine.cpp
    src/analysis/code_analyzer.cpp
    src/analysis/relationship_engine.cpp
    src/git/git_manager.cpp
    src/favorites/favorites_manager.cpp
    src/command/command_executor.cpp
    src/assets/image_manager.cpp
    src/ui/grid_view.cpp
    src/ui/list_view.cpp
    src/ui/detail_panel.cpp
    src/ui/favorites_dropdown.cpp
    src/ui/dual_pane_view.cpp
    src/ui/top_bar.cpp
    src/ui/command_panel.cpp
    src/ui/theme.cpp
    src/ui/tab_system.cpp
    src/ui/file_icon.cpp
    src/ui/context_menu.cpp
    external/imgui/misc/cpp/imgui_stdlib.cpp
    external/stb/stb_image.cpp
)

if(GRIDFIRE_BUILD_APPLICATION)
    list(APPEND GRIDFIRE_CORE_SOURCES src/ui/tab_system_render.cpp)
endif()

add_library(gridfire_core ${GRIDFIRE_CORE_SOURCES})
target_include_directories(gridfire_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/misc/cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
)
target_link_libraries(gridfire_core PUBLIC SQLite::SQLite3 Threads::Threads)

target_compile_options(gridfire_core PRIVATE -Wall -Wextra -Wpedantic)

if(GRIDFIRE_BUILD_APPLICATION)
    find_package(OpenGL REQUIRED)
    find_package(SDL2 REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GIT2 libgit2)
    pkg_check_modules(TREESITTER tree-sitter)

    add_executable(gridfire
        src/main.cpp
        external/imgui/imgui.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/backends/imgui_impl_sdl2.cpp
        external/imgui/backends/imgui_impl_opengl3.cpp
    )

    target_include_directories(gridfire PRIVATE
        external/imgui
        external/imgui/backends
        external/stb
        ${SDL2_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
        ${GIT2_INCLUDE_DIRS}
        ${TREESITTER_INCLUDE_DIRS}
    )

    target_link_libraries(gridfire PRIVATE
        gridfire_core
        OpenGL::GL
        SDL2::SDL2
        ${GIT2_LIBRARIES}
        ${TREESITTER_LIBRARIES}
    )
endif()

enable_testing()

add_executable(sqlite_manager_test tests/test_database.cpp)
target_link_libraries(sqlite_manager_test PRIVATE gridfire_core)
add_test(NAME sqlite_manager_test COMMAND sqlite_manager_test)

add_executable(filesystem_engine_test tests/test_filesystem.cpp)
target_link_libraries(filesystem_engine_test PRIVATE gridfire_core)
add_test(NAME filesystem_engine_test COMMAND filesystem_engine_test)

add_executable(tab_system_test tests/test_tabs.cpp)
target_link_libraries(tab_system_test PRIVATE gridfire_core)
add_test(NAME tab_system_test COMMAND tab_system_test)

add_executable(analysis_test tests/test_analysis.cpp)
target_link_libraries(analysis_test PRIVATE gridfire_core)
add_test(NAME analysis_test COMMAND analysis_test)
