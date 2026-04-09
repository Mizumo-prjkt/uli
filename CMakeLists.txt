cmake_minimum_required(VERSION 3.14)
project(uli CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ULI_DEBUG_MODE "Enable ULI debugging and hardware test mode" OFF)
if(ULI_DEBUG_MODE)
    add_compile_definitions(ULI_DEBUG_MODE)
endif()

# Build Options
option(ULI_BUILD_INSTALLER  "Build the core installer" ON)
option(ULI_BUILD_PATCHER    "Build the ISO patching tool" ON)
option(ULI_BUILD_COMPOSITOR "Build the GUI profile compositor" ON)

include(FetchContent)

# Find system dbus-1 library for native D-Bus communication
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

# Fetch yaml-cpp
FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        yaml-cpp-0.9.0
)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "Disable yaml-cpp tests" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "Disable yaml-cpp tools" FORCE)
FetchContent_MakeAvailable(yaml-cpp)

# Define our libraries
add_library(dpkg_apt installer/lib/package_mgr/dpkg_apt/dpkg_apt.cpp)
target_include_directories(dpkg_apt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/installer/lib/package_mgr/dpkg_apt)
target_link_libraries(dpkg_apt PUBLIC yaml-cpp)

add_library(alps_mgr installer/lib/package_mgr/alps/alps_mgr.cpp)
target_include_directories(alps_mgr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/installer/lib/package_mgr/alps)
target_link_libraries(alps_mgr PUBLIC yaml-cpp)

add_library(apk_mgr installer/lib/package_mgr/apk/apk_mgr.cpp)
target_include_directories(apk_mgr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/installer/lib/package_mgr/apk)
target_link_libraries(apk_mgr PUBLIC yaml-cpp)

# === ULI Installer (Core Binary) ===
pkg_check_modules(ARCHIVE libarchive)

if(ULI_BUILD_INSTALLER)
    add_executable(uli_installer 
        installer/main.cpp 
        installer/lib/package_mgr/indexer.cpp
        installer/lib/runtime/contents/profiles/profiles.cpp
        installer/lib/package_mgr/repofetch.cpp
    )
    target_include_directories(uli_installer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/installer ${DBUS_INCLUDE_DIRS})
    
    if(ARCHIVE_FOUND)
        target_include_directories(uli_installer PRIVATE ${ARCHIVE_INCLUDE_DIRS})
        target_link_libraries(uli_installer PRIVATE ${ARCHIVE_LIBRARIES})
        target_compile_definitions(uli_installer PRIVATE ULI_HAS_LIBARCHIVE)
    endif()

    target_link_libraries(uli_installer PRIVATE dpkg_apt alps_mgr apk_mgr yaml-cpp ${DBUS_LIBRARIES})
endif()

if(ULI_BUILD_COMPOSITOR)
    add_subdirectory(compositor)
endif()

# === ULI Patcher (ISO Injection Tool) ===
# Detect libisoburn system library for native xorriso API
pkg_check_modules(ISOBURN libisoburn-1)

if(ULI_BUILD_PATCHER)
    add_executable(uli_patcher
        patcher/main.cpp
        patcher/lib/iso_backend.cpp
        patcher/lib/boot_detector.cpp
        patcher/lib/boot_injector.cpp
        patcher/lib/autostart_gen.cpp
    )

    target_include_directories(uli_patcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/patcher)

    if(ISOBURN_FOUND)
        message(STATUS "libisoburn found — uli_patcher will use native xorriso API")
        target_compile_definitions(uli_patcher PRIVATE ULI_HAS_LIBISOBURN)
        target_include_directories(uli_patcher PRIVATE ${ISOBURN_INCLUDE_DIRS})
        target_link_libraries(uli_patcher PRIVATE ${ISOBURN_LIBRARIES})
    else()
        message(STATUS "libisoburn not found — uli_patcher will use xorriso CLI fallback")
    endif()
endif()

