cmake_minimum_required(VERSION 3.21)
project(rox_engine VERSION 0.1 LANGUAGES CXX)

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

# ====== Define Source Subdirectories ======
# List all your source subdirectories here
set(SOURCE_SUBDIRS
    formats
    logger
    math
    memory
    render
    resources
    scene
    system
    ui
)

# ====== Collect Source Files ======
set(SRC_FILES)

foreach(subdir IN LISTS SOURCE_SUBDIRS)
    # Define the relative path for source grouping
    set(relative_path "${subdir}")

    # Define the absolute path to the subdirectory
    # set(dir_path "${SOURCE_DIR}/${subdir}")

    # Add source group for IDE (e.g., Visual Studio)
    source_group(${relative_path} FILES)

    # Collect .h and .cpp files
    file(GLOB_RECURSE headers "${subdir}/*.h")
    file(GLOB_RECURSE sources "${subdir}/*.cpp")

    # If on Apple, also collect .mm files
    if(APPLE)
        file(GLOB_RECURSE objc_sources "${subdir}/*.mm")
        list(APPEND sources ${objc_sources})
    endif()

    # Append collected files to SRC_FILES
    list(APPEND SRC_FILES ${headers} ${sources})

    # Organize sources into groups
    source_group(${relative_path} FILES ${headers} ${sources})
endforeach()

# ======Define the Library ======
# Choose STATIC or SHARED based on your needs
add_library(${PROJECT_NAME} STATIC ${SRC_FILES})

# ======Include Directories ======
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${INCLUDE_DIR}
        # Add dependency include directories
        "D:/CLT/assimp/include/assimp"
        "D:/CLT/zlib"
        "D:/CLT/zlib/build"
        "D:/CLT/bullet3/build/src"
)

# ======Link Directories ======
# Prefer using full paths or imported targets in modern CMake
link_directories(
    "D:/CLT/assimp/lib"
    "D:/CLT/zlib/build/Release"
    "D:/CLT/bullet3/build/lib/Release"
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

# ======Link Libraries ======
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        rox_engine.lib

        glfw3.lib
        assimp-vc143-mt.lib
        SDL3.lib
        SDL3_test.lib
        Bullet3Collision.lib
        Bullet3Common.lib
        Bullet3Dynamics.lib
        LinearMath.lib
        # Add other dependencies here
)

# Link System Libraries (Windows)
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        opengl32.lib
        user32.lib
        gdi32.lib
        shell32.lib
    )
endif()