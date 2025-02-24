cmake_minimum_required(VERSION 3.21)
project(rox_engine VERSION 0.1 LANGUAGES C CXX)

# ====== Set C++ Standard ======
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ====== Detect Platforms ======
if(WIN32)
    # Windows-specific settings
    message(STATUS "Configuring for Windows")
elseif(APPLE)
    # macOS-specific settings
    message(STATUS "Configuring for macOS")
    # Set default configuration to Release for generated schemes
    set(CMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH "NO")
    set(CMAKE_XCODE_SCHEME_BUILD_CONFIGURATION "Release")
    set(CMAKE_CONFIGURATION_TYPES "Release;Debug" CACHE STRING "Available configurations" FORCE)
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

# ====== Include Directories ======
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Engine")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Engine/RoxCore")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/Engine/RoxGraphics")

# ====== Add Subdirectories ======
add_subdirectory(Dependencies)

# ====== Add Subdirectories ======
add_subdirectory(Engine)
add_subdirectory(Engine/RoxCore)
add_subdirectory(Engine/RoxGraphics)

# ======Link Directories ======
# Prefer using full paths or imported targets in modern CMake
link_directories(
    # Link Directories of External Libraries
    # Link Directories of Subdirectories
)

# ====== Platform-Specific Configurations ======
if(EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "--bind")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".wasm")
endif()

# ====== Handle Android Specifics ======
if(ANDROID)
    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    set_property(TARGET ${PROJECT_NAME} PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR ${ANDROID_PACKAGE_SOURCE_DIR})
    add_custom_target(cmake_android_package
        SOURCES
        ${ANDROID_PACKAGE_SOURCE_DIR}/AndroidManifest.xml
        ${ANDROID_PACKAGE_SOURCE_DIR}/build.gradle
        ${ANDROID_PACKAGE_SOURCE_DIR}/grable.properties
        ${ANDROID_PACKAGE_SOURCE_DIR}/res/values/libs.xml
    )

    # Include OpenSSL or other dependencies as needed
    include(FetchContent)
    FetchContent_Declare(
        android_openssl
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        URL https://github.com/KDAB/android_openssl/archive/refs/heads/master.zip
    )
    FetchContent_MakeAvailable(android_openssl)
    include(${android_openssl_SOURCE_DIR}/android_openssl.cmake)
endif()

# ====== Handle macOS Specifics ======
if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SRC_FILES})

# 
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ====== Set Output Directories ======
set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ====== Organize Folders in IDE ======
set_property(GLOBAL PROPERTY USE_FOLDERS ON)