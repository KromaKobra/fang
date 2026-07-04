cmake_minimum_required(VERSION 3.24)

project(Fang VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

find_package(Qt6 6.6 REQUIRED COMPONENTS
    Core Gui Qml Quick QuickControls2 Concurrent DBus)

qt_standard_project_setup(REQUIRES 6.6)

# toml++ — prefer a system/package-manager copy, fall back to FetchContent.
find_package(tomlplusplus QUIET)
if(NOT tomlplusplus_FOUND)
    include(FetchContent)
    FetchContent_Declare(tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tomlplusplus)
    FetchContent_MakeAvailable(tomlplusplus)
endif()

include(GNUInstallDirs)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)

# Desktop integration assets
install(FILES data/dev.kromakobra.Fang.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/dev.kromakobra.Fang.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES data/icons/dev.kromakobra.Fang.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
