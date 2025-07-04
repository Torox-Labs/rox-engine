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
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'memory' under 'Graphics' in the solution
set_target_properties(RoxMemory PROPERTIES FOLDER "Core")

