# ====== Define Sources ======
file(GLOB_RECURSE MATH_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(math STATIC ${MATH_SRC})

# ====== Include Directories ======
target_include_directories(math
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/math
)

set_target_properties(math PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'math' under 'Graphics' in the solution
set_target_properties(math PROPERTIES FOLDER "Graphics")

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${LOGGER_SRC})
