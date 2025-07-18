# ====== Define Sources ======
file(GLOB_RECURSE FORMATS_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(RoxFormats STATIC ${FORMATS_SRC})

# ====== Include Directories ======
target_include_directories(RoxFormats
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(RoxFormats PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SYSTEM_SRC})

# ====== Set Folder Properties for Visual Studio ======
# This nests 'formats' under 'Graphics' in the solution
set_target_properties(RoxFormats PROPERTIES FOLDER "Core")