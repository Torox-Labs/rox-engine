cmake_minimum_required(VERSION 3.21)
project(rox_engine VERSION 0.1 LANGUAGES CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    # Windows-specific settings
    message(STATUS "Configuring for Windows")
elseif(APPLE)
    # macOS-specific settings
    message(STATUS "Configuring for macOS")
elseif(UNIX)
    # Linux-specific settings
    message(STATUS "Configuring for Linux")
elseif(ANDROID)
    # Android-specific configuration here
    message(STATUS "Configuring for Android")
    add_definitions(-DANDROID_BUILD)
    target_link_libraries(${CMAKE_PROJECT_NAME} android_specific_library)
    set(ANDROID 1)
endif()

# Set Source and Include Directories
# set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
# set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/formats")
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/logger")
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/math")
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/memory")
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/render")
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/resources")
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/scene")
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/system")
set(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/ui")

# Collect Source Files
file(GLOB_RECURSE SRC_FILES
    "${SOURCE_DIR}/*.cpp"
    "${SOURCE_DIR}/*.h"
)

# If on Apple, include .mm files
if(APPLE)
    file(GLOB_RECURSE SRC_FILES
        ${SRC_FILES}
        "${SOURCE_DIR}/*.mm"
    )
endif()

# Define the Library
add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

# Include Directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${INCLUDE_DIR}
        # Add dependency include directories
        "D:/CLT/assimp/include/assimp"
        "D:/CLT/zlib"
        "D:/CLT/zlib/build"
        "D:/CLT/bullet3/build/src"
)

macro(define_source_files)
    set(temp_src_dir ${ARGN})
    source_group(${ARGN} ${ARGN})

    list(APPEND src_types "${temp_src_dir}/*.h" "${temp_src_dir}/*.cpp")

    if(APPLE)
        list(APPEND src_types "${temp_src_dir}/*.mm")
    endif()

    file(GLOB temp_src_list RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${src_types})
    list(APPEND src_files ${temp_src_list})
    unset(temp_src_dir)
    unset(temp_src_list)
endmacro()

if(EMSCRIPTEN)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_FLAGS -msse2)
endif()

define_source_files(formats)
define_source_files(logger)
define_source_files(math)
define_source_files(memory)
define_source_files(render)
define_source_files(resources)
define_source_files(scene)
define_source_files(system)
define_source_files(ui)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# if(EMSCRIPTEN)
#     add_library(${PROJECT_NAME} SHARED ${src_files})
# else()
    # add_library(${PROJECT_NAME} ${src_files})
# endif()

# Link Libraries
# target_link_libraries(${PROJECT_NAME}
#     PRIVATE
#         rox_engine.lib
#         glfw3.lib
#         opengl32.lib
#         user32.lib
#         gdi32.lib
#         shell32.lib
#         assimp-vc143-mt.lib
#         SDL3.lib
#         SDL3_test.lib
#         Bullet3Collision.lib
#         Bullet3Common.lib
#         Bullet3Dynamics.lib
#         LinearMath.lib
#         # Add other dependencies here
# )