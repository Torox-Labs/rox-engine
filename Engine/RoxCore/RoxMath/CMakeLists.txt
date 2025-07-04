# ====== Define Sources ======
file(GLOB_RECURSE MATH_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(RoxMath STATIC ${MATH_SRC})

# ====== Include Directories ======
target_include_directories(RoxMath
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(RoxMath PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
    PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'math' under 'Graphics' in the solution
set_target_properties(RoxMath PROPERTIES FOLDER "Core")
