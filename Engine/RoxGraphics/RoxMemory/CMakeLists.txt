# ====== Define Sources ======
file(GLOB_RECURSE MEMORY_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(RoxMemory STATIC ${MEMORY_SRC})

# ====== Include Directories ======
target_include_directories(RoxMemory
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(RoxMemory PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'memory' under 'Graphics' in the solution
set_target_properties(RoxMemory PROPERTIES FOLDER "Graphics")

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${MEMORY_SRC})
