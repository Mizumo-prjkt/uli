cmake_minimum_required(VERSION 3.16)
project(HarukaInstaller
    VERSION 0.1.0
    DESCRIPTION "HarukaInstaller - Yet another Linux Installer in C++"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Find ncurses ─────────────────────────────────────────────────────────────
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

# ── ncurses TUI library (object library, shared by all targets) ──────────────
set(NCURSES_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/ncurses)

add_library(ncurseslib OBJECT
    ${NCURSES_LIB_DIR}/ncurseslib.cpp
    ${NCURSES_LIB_DIR}/mainmenu/mainmenu.cpp
)
target_include_directories(ncurseslib PUBLIC
    ${NCURSES_LIB_DIR}
    ${NCURSES_LIB_DIR}/mainmenu
    ${NCURSES_LIB_DIR}/mainmenu/pages
    ${NCURSES_LIB_DIR}/test
)

# ── Test UI executable ───────────────────────────────────────────────────────
# Build with: cmake -DBUILD_TEST_UI=ON ..
option(BUILD_TEST_UI "Build the ncurses TUI test program" OFF)

if(BUILD_TEST_UI)
    add_executable(test_ui
        ${NCURSES_LIB_DIR}/test/testing.cpp
        $<TARGET_OBJECTS:ncurseslib>
    )
    target_compile_definitions(test_ui PRIVATE TESTUI)
    target_include_directories(test_ui PRIVATE
        ${NCURSES_LIB_DIR}
        ${NCURSES_LIB_DIR}/mainmenu
        ${NCURSES_LIB_DIR}/mainmenu/pages
        ${NCURSES_LIB_DIR}/test
    )
    target_link_libraries(test_ui PRIVATE ${CURSES_LIBRARIES})
endif()

# ── Main installer executable (placeholder) ──────────────────────────────────
# add_executable(haruka_installer main.cpp $<TARGET_OBJECTS:ncurseslib>)
# target_link_libraries(haruka_installer PRIVATE ${CURSES_LIBRARIES})
