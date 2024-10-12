# ====== Define Sources ======
file(GLOB_RECURSE FORMATS_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(formats STATIC ${FORMATS_SRC})

# ====== Include Directories ======
target_include_directories(formats
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${FORMATS_SRC})

set_target_properties(formats PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'formats' under 'Graphics' in the solution
set_target_properties(formats PROPERTIES FOLDER "Graphics")

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${FORMATS_SRC})