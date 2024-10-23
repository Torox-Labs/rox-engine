# ====== Define Sources ======
file(GLOB_RECURSE SCENE_SRC "*.cpp" "*.mm" "*.h")

# ====== Add Library ======
add_library(RoxScene STATIC ${SCENE_SRC})

# ====== Include Directories ======
target_include_directories(RoxScene
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(RoxScene PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'scene' under 'Graphics' in the solution
set_target_properties(RoxScene PROPERTIES FOLDER "Graphics")

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SCENE_SRC})
