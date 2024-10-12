# ====== Define Sources ======
file(GLOB_RECURSE SCENE_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(scene STATIC ${SCENE_SRC})

# ====== Include Directories ======
target_include_directories(scene
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(scene PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'scene' under 'Graphics' in the solution
set_target_properties(scene PROPERTIES FOLDER "Graphics")

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SCENE_SRC})
