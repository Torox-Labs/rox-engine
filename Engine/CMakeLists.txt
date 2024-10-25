# ====== Define Source Subdirectories ======
# set(SOURCE_SUBDIRS
#     RoxFormats
#     RoxLogger
#     RoxMath
#     RoxMemory
#     RoxRender
#     RoxResources
#     RoxScene
#     RoxSystem
# )

# # ====== Collect Source Files ======
# set(SRC_FILES)

# foreach(subdir IN LISTS SOURCE_SUBDIRS)
#     # Add the subdirectory
#     add_subdirectory(${subdir})
# endforeach()

# ====== Define the Main Library ======
# Choose STATIC or SHARED based on your needs
add_library(${PROJECT_NAME} SHARED rox_engine.cpp)

# ====== Include Directories ======
target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "D:/CLT/assimp/include/assimp"
    "D:/CLT/zlib"
    "D:/CLT/zlib/build"
    "D:/CLT/bullet3/build/src"
    "D:/CLT/OpenGL/glfw_3.3.9/include/GLFW"
)

# ====== Link Libraries ======
# target_link_libraries(${PROJECT_NAME}
#     PRIVATE
#     RoxFormats
#     RoxLogger
#     RoxMath
#     RoxMemory
#     RoxRender
#     RoxResources
#     RoxScene
#     RoxSystem

#     # External Libraries
# )

# ====== Link System Libraries (Windows) ======
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw3.lib
        opengl32.lib
        user32.lib
        gdi32.lib
        shell32.lib
    )
endif()

# ====== Organize Sources in IDE ======
# (Optional) You can remove or adjust this if not used
# source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX "Graphics" FILES ${SRC_FILES})
