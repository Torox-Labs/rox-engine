# ====== Define Source Subdirectories ======
set(SOURCE_SUBDIRS
    formats
    logger
    math
    
)

# ====== Collect Source Files ======
set(SRC_FILES)

# foreach(subdir IN LISTS SOURCE_SUBDIRS)
#     # Define the relative path for source grouping
#     set(relative_path "${subdir}")

#     # Define the absolute path to the subdirectory
#     # set(dir_path "${SOURCE_DIR}/${subdir}")

#     # Add source group for IDE (e.g., Visual Studio)
#     source_group(${relative_path} FILES)

#     # Collect .h and .cpp files
#     file(GLOB_RECURSE headers "${subdir}/*.h")
#     file(GLOB_RECURSE sources "${subdir}/*.cpp")

#     # If on Apple, also collect .mm files
#     if(APPLE)
#         file(GLOB_RECURSE objc_sources "${subdir}/*.mm")
#         list(APPEND sources ${objc_sources})
#     endif()

#     # Append collected files to SRC_FILES
#     list(APPEND SRC_FILES ${headers} ${sources})

#     # Organize sources into groups
#     source_group(${relative_path} FILES ${headers} ${sources})
# endforeach()

foreach(subdir IN LISTS SOURCE_SUBDIRS)
    # Add the subdirectory
    add_subdirectory(${subdir})

    # Collect source files from each subdirectory's CMakeLists.txt
    # The subdirectories will define their own targets, which can be linked here
endforeach()

# ====== Define the Main Library ======
# Choose STATIC or SHARED based on your needs
# add_library(${PROJECT_NAME} STATIC ${SRC_FILES})
add_library(${PROJECT_NAME} SHARED rox_engine.cpp)


# ====== Include Directories ======
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        "D:/CLT/assimp/include/assimp"
        "D:/CLT/zlib"
        "D:/CLT/zlib/build"
        "D:/CLT/bullet3/build/src"
)

# ====== Link Libraries ======
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        formats
        logger
        math
        # External Libraries
        rox_engine.lib
        glfw3.lib
        assimp-vc143-mt.lib
        SDL3.lib
        SDL3_test.lib
        Bullet3Collision.lib
        Bullet3Common.lib
        Bullet3Dynamics.lib
        LinearMath.lib
)

# ====== Link System Libraries (Windows) ======
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        opengl32.lib
        user32.lib
        gdi32.lib
        shell32.lib
    )
endif()

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SRC_FILES})