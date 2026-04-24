cmake_minimum_required(VERSION 3.16)

# ── Parse VERSION file ───────────────────────────────────────────────────────
# Read version from the VERSION file at project root
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VERSION_LINES)
foreach(LINE ${VERSION_LINES})
    if(LINE MATCHES "^installer=\"([^\"]+)\"")
        set(HARUKA_VERSION "${CMAKE_MATCH_1}")
    elseif(LINE MATCHES "^license=\"([^\"]+)\"")
        set(HARUKA_LICENSE "${CMAKE_MATCH_1}")
    elseif(LINE MATCHES "^author=\"([^\"]+)\"")
        set(HARUKA_AUTHOR "${CMAKE_MATCH_1}")
    endif()
endforeach()

if(NOT HARUKA_VERSION)
    set(HARUKA_VERSION "dev")
endif()
if(NOT HARUKA_LICENSE)
    set(HARUKA_LICENSE "MIT")
endif()
if(NOT HARUKA_AUTHOR)
    set(HARUKA_AUTHOR "unknown")
endif()

message(STATUS "HarukaInstaller v${HARUKA_VERSION} (License: ${HARUKA_LICENSE}, Author: ${HARUKA_AUTHOR})")

project(HarukaInstaller
    VERSION ${HARUKA_VERSION}
    DESCRIPTION "HarukaInstaller - Yet another Linux Installer in C++"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Version defines passed to all targets
add_compile_definitions(
    VERSION_ID="${HARUKA_VERSION}"
    VERSION_LICENSE="${HARUKA_LICENSE}"
    VERSION_AUTHOR="${HARUKA_AUTHOR}"
)

# ── Find ncurses ─────────────────────────────────────────────────────────────
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

# ── ncurses TUI library (object library) ─────────────────────────────────────
set(INSTALLER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/installer)
set(NCURSES_LIB_DIR ${INSTALLER_DIR}/lib/ncurses)

add_library(ncurseslib OBJECT
    ${NCURSES_LIB_DIR}/ncurseslib.cpp
    ${NCURSES_LIB_DIR}/mainmenu/mainmenu.cpp
)
target_include_directories(ncurseslib PUBLIC
    ${NCURSES_LIB_DIR}
    ${NCURSES_LIB_DIR}/mainmenu
    ${NCURSES_LIB_DIR}/mainmenu/pages
    ${NCURSES_LIB_DIR}/test
    ${NCURSES_LIB_DIR}/help
    ${NCURSES_LIB_DIR}/versioning
)

# ── Test UI executable ───────────────────────────────────────────────────────
option(BUILD_TEST_UI "Build the ncurses TUI test program" ON)

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
        ${NCURSES_LIB_DIR}/help
        ${NCURSES_LIB_DIR}/versioning
    )
    target_link_libraries(test_ui PRIVATE ${CURSES_LIBRARIES})
endif()

# ── Main installer executable (placeholder) ──────────────────────────────────
# add_executable(haruka_installer ${INSTALLER_DIR}/main.cpp $<TARGET_OBJECTS:ncurseslib>)
# target_link_libraries(haruka_installer PRIVATE ${CURSES_LIBRARIES})
