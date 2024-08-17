cmake_minimum_required(VERSION 3.19)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release})
    add_compile_options(-O3)
endif()

add_compile_options(-Wall -Werror -pedantic)

project(scripts LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_supported OUTPUT error)

if(IPO_supported)
    macro(enable_lto target)
        set_property(TARGET ${target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "${target}: IPO / LTO enabled")
    endmacro(enable_lto)
else()
    macro(enable_lto target)
        message(STATUS "${target}: IPO / LTO not supported: <${error}>")
    endmacro(enable_lto)
endif()

find_package(stduuid REQUIRED) # rename2uniq
include_directories(${stduuid_INCLUDE_DIRS})

find_package(nlohmann_json REQUIRED)
include_directories(${nlohmann_json_INCLUDE_DIRS})

# target_link_libraries(fixname PUBLIC glfw GLESv2 cpr)

add_executable(fixname src/fixname.cc)
enable_lto(fixname)

add_executable(rename2uniq src/rename2uniq.cc)
enable_lto(rename2uniq)

add_executable(mirror-git-repos src/mirror-git-repos.cc)
enable_lto(mirror-git-repos)

add_executable(archive_yt_playlist src/archive_yt_playlist.cc)
enable_lto(archive_yt_playlist)

if(NOT CMAKE_INSTALL_BINDIR)
    set(INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin)
elseif(CMAKE_INSTALL_BINDIR)
    set(INSTALL_DIR ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS
    fixname
    rename2uniq
    mirror-git-repos
    archive_yt_playlist
    RUNTIME DESTINATION ${INSTALL_DIR})
