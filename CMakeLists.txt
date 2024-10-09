cmake_minimum_required(VERSION 3.21)
project(rox_engine VERSION 0.1 LANGUAGES CXX)

if(${CMAKE_SYSTEM_NAME} MATCHES "Android")
   set(ANDROID 1)
endif()

# if(WIN32)
#     # Windows-specific settings
#     message(STATUS "Configuring for Windows")
# elseif(APPLE)
#     # macOS-specific settings
#     message(STATUS "Configuring for macOS")
# elseif(UNIX)
#     # Linux-specific settings
#     message(STATUS "Configuring for Linux")
# endif()

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
    set (CMAKE_CXX_STANDARD 11)
    set( CMAKE_CXX_FLAGS  -msse2 )
endif()

define_source_files(formats)
define_source_files(log)
define_source_files(math)
define_source_files(memory)
define_source_files(render)
define_source_files(resources)
define_source_files(scene)
define_source_files(system)
define_source_files(ui)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(EMSCRIPTEN)
   add_library(${CMAKE_PROJECT_NAME} SHARED ${src_files})
else()
   add_library(${CMAKE_PROJECT_NAME} ${src_files})
endif()
