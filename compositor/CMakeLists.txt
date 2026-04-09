cmake_minimum_required(VERSION 3.16)
project(uli_compositor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets)

# Find system libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBARCHIVE REQUIRED libarchive)

# Find yaml-cpp
if(NOT TARGET yaml-cpp)
    find_package(yaml-cpp REQUIRED)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/components
    ${CMAKE_BINARY_DIR}/generated
    ${LIBARCHIVE_INCLUDE_DIRS}
)

# Source files
set(COMPOSITOR_SOURCES
    main.cpp
    lib/components/fetchers/RepoFetcher.cpp
    lib/components/fetchers/arch/ArchFetcher.cpp
    lib/components/fetchers/alpine/AlpineFetcher.cpp
    lib/components/fetchers/debuntu/DebianFetcher.cpp
    lib/components/language/language_core.cpp
    lib/components/language/TranslationCompositor.cpp
    lib/components/categorization/categorization.cpp
    lib/components/categorization/lib/PackageMapper.cpp
    lib/components/yaml/yamlgen.cpp
    lib/components/yaml/YamlGenerator.cpp
    lib/components/storage/StorageModel.cpp
    lib/components/SyncManager.cpp
    lib/components/ProfileExporter.cpp
    lib/components/ProfileImporter.cpp
    lib/ui/Main_Menu/Main_Menu.cpp
    lib/ui/ProfileEditor/ProfileEditor.cpp
    lib/ui/ProfileEditor/UserDialog.cpp
    lib/ui/ProfileEditor/PartitionDialog.cpp
    lib/ui/ProfileEditor/DiskGraphWidget.cpp
)

add_executable(uli_compositor ${COMPOSITOR_SOURCES})

# Link libraries
target_link_libraries(uli_compositor PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
    yaml-cpp
    ${LIBARCHIVE_LIBRARIES}
)
